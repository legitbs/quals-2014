## Generated from include/choripan.proto for 
require "beefcake"

module Choripan
  module Messages

    class Hello
      include Beefcake::Message
    end

    class Ready
      include Beefcake::Message
    end

    class GetPostReq
      include Beefcake::Message
    end

    class GetPostResp
      include Beefcake::Message
    end

    class ListPostReq
      include Beefcake::Message
    end

    class ListPostResp
      include Beefcake::Message
    end

    class ListLogReq
      include Beefcake::Message
    end

    class ListLogResp
      include Beefcake::Message
    end

    class ErrorResp
      include Beefcake::Message
    end

    class Post
      include Beefcake::Message
    end

    class Log
      include Beefcake::Message
    end

    class Signature
      include Beefcake::Message
    end

    class PubKey
      include Beefcake::Message
    end

    class Hello
      required :uuid, :bytes, 1
    end

    class Ready
    end

    class GetPostReq
      required :uuid, :bytes, 1
      required :signature, Signature, 2
    end

    class GetPostResp
      optional :post, Post, 1
    end

    class ListPostReq
    end

    class ListPostResp
      repeated :uuids, :bytes, 1
    end

    class ListLogReq
    end

    class ListLogResp
      repeated :logs, Log, 1
    end

    class ErrorResp
      required :reason, :bytes, 1
    end

    class Post
      optional :uuid, :bytes, 1
      required :title, :bytes, 2
      required :body, :bytes, 3
    end

    class Log
      required :uuid, :bytes, 1
      required :signature, Signature, 2
      required :timestamp, :uint64, 3
    end

    class Signature
      required :r, :bytes, 1
      required :s, :bytes, 2
      required :k, PubKey, 3
    end

    class PubKey
      required :groupname, :bytes, 1
      required :x, :bytes, 2
      required :y, :bytes, 3
    end
  end
end
