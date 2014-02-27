task :default => :pb_defs

desc "Turn protobuffs into messages"
task :pb_defs => 'lib/fritas/messages.rb'

desc "Package parts for distribution"
task :dist => ['dist/fritas/fritas.proto']

file 'lib/fritas/messages.rb' => 'include/fritas.proto' do |t|
  sh "export BEEFCAKE_NAMESPACE=Medianoche::Messages; protoc --beefcake_out lib/fritas #{t.prerequisites.first}"
  sh "mv lib/fritas/fritas.pb.rb #{t.name}"
end

desc "Package parts for distribution"
task :dist => ['dist/fritas/fritas.proto']

directory 'dist/fritas'

file 'dist/fritas/fritas.proto' => ['include/fritas.proto', 'dist/fritas'] do |t|
  sh "cp #{t.prerequisites.first} #{t.name}"
end
