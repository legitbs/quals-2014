task :default => :pb_defs

desc "Turn protobuffs into messages"
task :pb_defs => 'lib/fritas/messages.rb'

desc "Package parts for distribution to contestants"
task :dist => ['dist/fritas/fritas.proto',
               'dist/fritas/fritas.csv'
              ]

require 'warbler'
desc "Package the executables into a jar"
Warbler::Task.new

file 'lib/fritas/messages.rb' => 'include/fritas.proto' do |t|
  sh "export BEEFCAKE_NAMESPACE=Fritas::Messages; protoc --beefcake_out lib/fritas #{t.prerequisites.first}"
  sh "mv lib/fritas/fritas.pb.rb #{t.name}"
end

directory 'dist/fritas'

file 'dist/fritas/fritas.proto' => ['include/fritas.proto', 'dist/fritas'] do |t|
  sh "cp #{t.prerequisites.first} #{t.name}"
end

file 'dist/fritas/fritas.csv' => ['include/fritas.csv', 'dist/fritas'] do |t|
  sh "cp #{t.prerequisites.first} #{t.name}"
end
