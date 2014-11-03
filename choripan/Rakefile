task :default => :pb_defs

desc "Turn protobuffs into messages"
task :pb_defs => 'lib/choripan/messages.rb'

desc "Package parts for distribution"
task :dist => ['dist/choripan/choripan.proto']

file 'lib/choripan/messages.rb' => 'include/choripan.proto' do |t|
  sh "export BEEFCAKE_NAMESPACE=Choripan::Messages; protoc --beefcake_out lib/choripan #{t.prerequisites.first}"
  sh "mv lib/choripan/choripan.pb.rb #{t.name}"
end

desc "Package parts for distribution"
task :dist => ['dist/choripan/choripan.proto']

directory 'dist/choripan'

file 'dist/choripan/choripan.proto' => ['include/choripan.proto', 'dist/choripan'] do |t|
  sh "cp #{t.prerequisites.first} #{t.name}"
end

require 'warbler'
desc "Package the executables into a jar"
Warbler::Task.new