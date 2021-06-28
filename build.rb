#!/usr/bin/env ruby

require 'fileutils'
require 'open3'
require 'optparse'
require 'pathname'
require 'rbconfig'

COMPILE_COMMANDS_FILE = 'compile_commands.json'
POSSIBLE_BUILD_TYPES = %w[Debug Release RelWithDebInfo MinSizeRel]

class OmulatorBuilder
  def initialize(**kwargs)
    @build_type = kwargs[:build_type] || POSSIBLE_BUILD_TYPES.first
    @notests    = kwargs[:notests]    || false
    @testnames  = kwargs[:testnames]  || nil
    @toolchain  = kwargs[:toolchain]  || default_toolchain
    @verbose    = kwargs[:verbose]    || false

    @build_dir  = File.join OUTPUT_DIR, @toolchain, @build_type
    @proj_dir = __dir__
  end

  # Perform static analysis. For best results, don't build tests!
  def analyze
    # First, update build if necessary
    build
    Dir.chdir @build_dir
    spawn_cmd "run-clang-tidy -p '#{@proj_dir}/#{@build_dir}' -header-filter='\.hpp'"
    Dir.chdir @proj_dir
  end

  # Basic cmake build
  def build(**kwargs)
    spawn_cmd "cmake -B #{@build_dir} -GNinja -DCMAKE_BUILD_TYPE=#{@build_type} "\
      "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON "\
      "-DOMULATOR_BUILD_TESTS=#{@notests ? 'OFF' : 'ON'} "\
      "#{toolchain_args} #{kwargs[:addl_cmake_args]}"\
      "&& cmake --build #{@build_dir} -j #{kwargs[:addl_cmake_bld_args]} "\
      "#{'-v' if verbose?} -- #{kwargs[:addl_generator_args]}"
    # CMake screws up the permissions for executables with the msvc-wsl toolchain
    spawn_cmd "find #{@build_dir} -iname *.exe | xargs chmod 755" if @toolchain == "msvc-wsl"

    # Make compile_commands.json available for tools that need it in the working directory
    begin
      File.delete(COMPILE_COMMANDS_FILE) if File.exist?(COMPILE_COMMANDS_FILE)
      FileUtils.cp("#{@build_dir}/#{COMPILE_COMMANDS_FILE}", COMPILE_COMMANDS_FILE)
    rescue => msg
      puts "Unable to copy compile_commands.json to the current directory (#{msg})"
    end
  end

  # Have the generator perform a clean
  def clean
    build addl_cmake_bld_args: '--target clean'
  end

  # Delete _ALL_ build products (i.e. CMake cache in addition to build products)
  def cleanall
    FileUtils.rm_r(OUTPUT_DIR, force: true, verbose: true)
  end

  # Same as build, except have the generator perform a clean first
  def rebuild
    build addl_cmake_bld_args: '--clean-first'
  end

  # Rerun any tests that previously failed
  def retest
    Dir.chdir @build_dir
    spawn_cmd "ctest #{'-VV' if verbose?} -j --rerun-failed --output-on-failure"
    Dir.chdir @proj_dir
  end

  def test
    # First, update build if necessary
    build
    Dir.chdir @build_dir
    if @testnames != nil
      testnamestring = "-R '#{@testnames.join('|')}'"
    else
      testnamestring = nil
    end
    spawn_cmd "ctest #{'-VV' if verbose?} #{testnamestring if @testnames} -j --output-on-failure --schedule-random --repeat-until-fail 3"
    Dir.chdir @proj_dir
  end

  private

  OUTPUT_DIR = 'build'

  def default_toolchain
    os = RbConfig::CONFIG['host_os']
    case os
    when /mswin/
      "msvc"
    when /linux/
      "gcc"
    else
      raise "Cannot infer toolchain for OS '#{os}'; please supply a --toolchain argument"
    end
  end

  def spawn_cmd(cmd)
    # spawn a subprocess with stdout and stderr merged, and block till it's done
    puts "->'#{cmd}'"
    Open3.popen2e cmd do |stdin, stdout_err, wait_thr|
      while line = stdout_err.gets
        puts line
      end

      unless wait_thr.value.success?
        puts "Build command failed!"
        abort
      end
    end
  end

  def toolchain_args
    if @toolchain
      args = "-DCMAKE_TOOLCHAIN_FILE=#{@proj_dir}/cmake/Toolchain-#{@toolchain}.cmake"
      if @toolchain == "clang-cl-wsl"
        args += " -DHOST_ARCH=#{ENV['WSL_HOST_ARCH']} "\
          "-DLLVM_NATIVE_TOOLCHAIN=#{ENV['WSL_LLVM_NATIVE_TOOLCHAIN']} "\
          "-DMSVC_BASE=#{ENV['WSL_MSVC_BASE']} "\
          "-DWINSDK_BASE=#{ENV['WSL_WINSDK_BASE']} "\
          "-DWINSDK_VER=#{ENV['WSL_WINSDK_VER']}"
      end
      args
    else
      ""
    end
  end

  def verbose?
    !!@verbose
  end
end

# Determine which methods are specific to OmulatorBuilder by diffing its public methods
# with those of its parent
POSSIBLE_ACTIONS =
  (OmulatorBuilder.public_instance_methods - OmulatorBuilder.superclass.public_instance_methods)
  .map!(&:to_s)
POSSIBLE_TOOLCHAINS = %w[msvc gcc clang clang-cl clang-cl-wsl msvc-wsl]

def main()
  pn = Pathname.new($0)
  Dir.chdir(pn.dirname)

  cmds = []
  options = {}

  OptionParser.new do |opts|
    opts.banner = <<~EOF
      This script is a wrapper around the CMake build process
      Usage: build.rb [options] [actions]
      Possible actions: [#{POSSIBLE_ACTIONS.sort.join('|')}]
      The 'build' action will be performed by default if no actions are given
    EOF

    opts.on('-b <BUILD_TYPE>', '--build-type <BUILD_TYPE>', POSSIBLE_BUILD_TYPES,
            "Specify the CMake build type [#{POSSIBLE_BUILD_TYPES.join('|')}]. "\
            "Also attempts to copy #{COMPILE_COMMANDS_FILE} to the current directory") do |build_type|
      options[:build_type] = build_type
    end

    opts.on('--debug', 'Perform a debug build; same as --build-type Debug') do
      options[:build_type] = "Debug"
    end

    opts.on('--release', 'Perform a release build; same as --build-type Release') do
      options[:build_type] = "Release"
    end

    opts.on('-h', '--help', 'Prints this help') do
      puts opts
      exit
    end

    opts.on('--testnames <TESTNAME1;TESTNAME2;etc...>', 'Specify the name(s) of a test to run, separated by semicolons') do |testnames|
      options[:testnames] = options[:testnames] || []
      options[:testnames].concat(testnames.split(';').map(&:strip))
    end

    opts.on('--notests', 'Do not build tests') do |v|
      options[:notests] = v
    end

    opts.on('-t <TOOLCHAIN>', '--toolchain <TOOLCHAIN>', POSSIBLE_TOOLCHAINS,
            "Specify the toolchain [#{POSSIBLE_TOOLCHAINS.join('|')}]") do |toolchain|
      options[:toolchain] = toolchain
    end

    opts.on('-v', '--verbose', 'Print extra info for all actions') do |v|
      options[:verbose] = v
    end
  end.parse!

  ARGV.each do |potential_action|
    cmds << potential_action if POSSIBLE_ACTIONS.include?(potential_action)
  end

  ob = OmulatorBuilder.new(**options)

  cmds << 'build' if cmds.empty?

  # Verify all commands are valid before executing them
  cmds.each do |cmd|
    unless OmulatorBuilder.method_defined? cmd.to_sym
      puts "Unknown build command: #{cmd}"
      exit
    end
  end

  cmds.each { |cmd| ob.send cmd.to_sym }

  puts "All build commands succeeded!"
end

main if __FILE__ == $0
