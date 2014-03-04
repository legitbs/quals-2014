task :default => :pb_defs

desc "Turn protobuffs into messages"
task :pb_defs => 'lib/medianoche/messages.rb'

desc "Package parts for distribution"
task :dist => ['dist/medianoche/medianoche.proto']

file 'lib/medianoche/messages.rb' => 'include/medianoche.proto' do |t|
  sh "export BEEFCAKE_NAMESPACE=Medianoche::Messages; protoc --beefcake_out lib/medianoche #{t.prerequisites.first}"
  sh "mv lib/medianoche/medianoche.pb.rb #{t.name}"
end

desc "Package parts for distribution"
task :dist => ['dist/medianoche/medianoche.proto']

directory 'dist/medianoche'

file 'dist/medianoche/medianoche.proto' => ['include/medianoche.proto', 'dist/medianoche'] do |t|
  sh "cp #{t.prerequisites.first} #{t.name}"
end
