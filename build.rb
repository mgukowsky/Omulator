#!/usr/bin/env ruby

require 'fileutils'
require 'open3'
require 'optparse'
require 'pathname'

POSSIBLE_ACTIONS = %w[build rebuild clean cleanall test]
POSSIBLE_TOOLCHAINS = %i[msvc gcc clang clang_cl]
TOOLCHAIN_TOOLS = {
  msvc:     %w[cl.exe cl.exe link.exe],
  gcc:      %w[gcc g++ ld],
  clang:    %w[clang clang++ lld],
  clang_cl: %w[clang-cl.exe clang-cl.exe lld-link.exe] 
}

class OmulatorBuilder
  @@GENERATOR = 'Ninja'
  attr_accessor :build_dir, :verbose

  def initialize(**kwargs)
    @build_dir  = kwargs[:build_dir]  || 'build'
    @verbose    = kwargs[:verbose]    || false

    if kwargs[:toolchain]
      set_toolchain(*TOOLCHAIN_TOOLS[kwargs[:toolchain]])
    end
  end

  # Basic cmake build
  def build(**kwargs)
    spawn_cmd "cmake -B #{@build_dir} -G#{@@GENERATOR} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON #{@toolchain_args} #{kwargs[:addl_cmake_args]}"\
      "&& cmake --build #{@build_dir} -j #{kwargs[:addl_cmake_bld_args]} "\
      "-- #{'-v' if verbose?} #{kwargs[:addl_generator_args]}"
  end

  # Same as build, except have the generator perform a clean first
  def rebuild 
    build addl_cmake_bld_args: '--clean-first'
  end

  # Have the generator perform a clean
  def clean
    build addl_cmake_bld_args: '--target clean'
  end

  # Delete _ALL_ build products (i.e. CMake cache in addition to build products)
  def cleanall
    FileUtils.rm_r(@build_dir, force: true, verbose: true)
  end

  def test
    Dir.chdir "#{@build_dir}"
    spawn_cmd "ctest #{'-V' if verbose?} -j --schedule-random --repeat-until-fail 3"
    Dir.chdir '..'
  end
  
  def verbose?
    !!@verbose
  end

  private

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

  def set_toolchain(cc, cxx, ld)
    @toolchain_args = "-DCMAKE_C_COMPILER=#{cc} -DCMAKE_CXX_COMPILER=#{cxx} -DCMAKE_LINKER=#{ld}"
  end
end

def main()
  pn = Pathname.new($0)
  Dir.chdir(pn.dirname) 

  cmds = []
  options = {}

  OptionParser.new do |opts|
    opts.banner = <<~EOF
      This script is a wrapper around the CMake build process 
      Usage: build.rb [options] [rules]
      Possible actions: #{POSSIBLE_ACTIONS}
      The 'build' action will be performed by default if no actions are given
    EOF

    opts.on('-h', '--help', 'Prints this help') do
      puts opts
      exit
    end

    opts.on('--toolchain [TOOLCHAIN]', POSSIBLE_TOOLCHAINS, 
            "Specify the toolchain [#{POSSIBLE_TOOLCHAINS.collect(&:to_s).join('|')}]") do |toolchain|
      options[:toolchain] = toolchain
    end

    opts.on('-v', '--verbose', 'Print extra info for all actions') do |v|
      options[:verbose] = v
    end
  end.parse!
  
  ARGV.each do |potential_action|
    cmds << potential_action if POSSIBLE_ACTIONS.include?(potential_action)
  end
  
  ob = OmulatorBuilder.new(options)

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

