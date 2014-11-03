# medianoche

blog backend api, with protocol buffers

1. store posts
2. posts can be queried by tag
3. tags with certain prefixes require exact match
4. tags can be listed, but password= tags aren't in the list
5. tags can be queried for correlation, but filtering's busted lol
6. password tag can be found
7. post can be retrieved with said password

## compiling and running with jruby

```sh
rake jar
java -jar medianoche.jar
```

## configuration

if the defaults don't work, use environment variables

```ruby
address = ENV['MEDIANOCHE_ADDRESS'] || '0.0.0.0'
port = ENV['MEDIANOCHE_PORT'] ? ENV['MEDIANOCHE_PORT'].to_i : 6909
$key = ENV['MEDIANOCHE_KEY'] || 'capa meets the sun to heal TrewpOjna'
```
