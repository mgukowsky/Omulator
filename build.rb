#!/usr/bin/env ruby

require 'open3'
require 'pathname'

BUILD_DIR = 'build'
GENERATOR = 'Ninja'

# Use either the ninja or make flag for a verbose build
GENERATOR_FLAGS = GENERATOR == 'Ninja' ? '-v' : 'V=1'

class OmulatorBuilder
  # Basic cmake build
  def build(**kwargs)
    spawn_cmd "cmake -B #{BUILD_DIR} -G#{GENERATOR} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON #{kwargs[:addl_cmake_args] || ''}"\
      "&& cmake --build #{BUILD_DIR} -j #{kwargs[:addl_cmake_bld_args] || ''} "\
      "-- #{GENERATOR_FLAGS} #{kwargs[:addl_generator_args] || ''}"
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
    spawn_cmd "rm -rf #{BUILD_DIR}"
  end

  def test
    Dir.chdir "#{BUILD_DIR}"
    spawn_cmd 'ctest -V -j --schedule-random --repeat-until-fail 3'
    Dir.chdir '..'
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
end

def main()
  pn = Pathname.new($0)
  Dir.chdir(pn.dirname) 

  cmds = []

  cmds << 'build' if ARGV.empty?
  cmds.concat ARGV

  # Verify all commands are valid before executing them
  cmds.each do |cmd|
    unless OmulatorBuilder.method_defined? cmd.to_sym
      puts "Unknown build command: #{cmd}"
      exit
    end
  end

  ob = OmulatorBuilder.new

  cmds.each { |cmd| ob.send cmd.to_sym }

  puts "All build commands succeeded!"
end

main if __FILE__ == $0

