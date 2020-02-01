#!/usr/bin/env ruby

require 'open3'
require 'pathname'

class OmulatorBuilder
  def build
    spawn_cmd 'cmake -B build -GNinja && cmake --build build -j -- -v' 
  end

  def test
    Dir.chdir 'build'
    spawn_cmd 'ctest -V -j --schedule-random --repeat-until-fail 3'
    Dir.chdir '..'
  end

  private

  def spawn_cmd(cmd)
    # spawn a subprocess with stdout and stderr merged, and block till it's done
    Open3.popen2e cmd do |stdin, stdout_err, wait_thr|
      while line = stdout_err.gets
        puts line
      end

      abort unless wait_thr.value.success?
    end
  end
end

def main()
  pn = Pathname.new($0)
  Dir.chdir(pn.dirname) 

  cmds = []

  if ARGV.empty?
    cmds << "build"
  else
    cmds.concat ARGV
  end

  # Verify all commands are valid before executing them
  cmds.each do |cmd|
    unless OmulatorBuilder.method_defined? cmd.to_sym
      puts "Unknown build command: #{cmd}"
      exit
    end
  end

  ob = OmulatorBuilder.new

  cmds.each { |cmd| ob.send cmd.to_sym }
end

main if __FILE__ == $0

