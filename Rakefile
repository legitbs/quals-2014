task :default => :pb_defs

task :pb_defs => 'lib/fritas/messages.rb'

file 'lib/fritas/messages.rb' => 'include/fritas.proto' do
  sh "export BEEFCAKE_NAMESPACE=Fritas::Messages; protoc --beefcake_out lib/fritas include/fritas.proto"
  sh "mv lib/fritas/fritas.pb.rb lib/fritas/messages.rb"
end