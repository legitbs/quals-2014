## Generated from include/fritas.proto for 
require "beefcake"

module Fritas
  module Messages

    class Hello
      include Beefcake::Message
    end

    class Ready
      include Beefcake::Message
    end

    class StorePostReq
      include Beefcake::Message
    end

    class StorePostResp
      include Beefcake::Message
    end

    class GetPostReq
      include Beefcake::Message
    end

    class GetPostResp
      include Beefcake::Message
    end

    class ErrorResp
      include Beefcake::Message
    end

    class Post
      include Beefcake::Message
    end

    class Tag
      include Beefcake::Message
    end

    class Hello
      required :uuid, :bytes, 1
    end

    class Ready
    end

    class StorePostReq
      required :post, Post, 1
    end

    class StorePostResp
      required :uuid, :bytes, 1
    end

    class GetPostReq
      required :uuid, :bytes, 1
    end

    class GetPostResp
      required :post, Post, 1
    end

    class ErrorResp
      required :reason, :bytes, 1
    end

    class Post
      optional :uuid, :bytes, 1
      required :title, :bytes, 2
      required :body, :bytes, 3
      repeated :tag, Tag, 4
    end

    class Tag
      optional :uuid, :bytes, 1
      required :tagname, :bytes, 2
    end
  end
end
