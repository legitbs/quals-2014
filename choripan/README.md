# choripan

ps3 style elliptic curve fuckery

1. get listing of entries
2. get log of recent signatures that were made with the same nonce
3. reverse the nonce into the private key
4. make a signature for key
5. get key

## configuration

choripan uses environment variables, these are the defaults:

```ruby
address = ENV['CHORIPAN_ADDRESS'] || '0.0.0.0'
port = ENV['CHORIPAN_PORT'] ? ENV['CHORIPAN_PORT'].to_i : 6910
$key = ENV['CHORIPAN_KEY'] || 'little black submarines Botniheg7'
```
