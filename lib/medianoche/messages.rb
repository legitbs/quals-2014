## Generated from include/medianoche.proto for 
require "beefcake"

module Medianoche
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

    class ListPostReq
      include Beefcake::Message
    end

    class ListPostResp
      include Beefcake::Message
    end

    class ListTagReq
      include Beefcake::Message
    end

    class ListTagResp
      include Beefcake::Message
    end

    class RelatedTagReq
      include Beefcake::Message
    end

    class RelatedTagResp
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
      optional :password, :bytes, 2
    end

    class GetPostResp
      optional :post, Post, 1
    end

    class ListPostReq
      optional :tag, Tag, 1
    end

    class ListPostResp
      repeated :uuids, :bytes, 1
    end

    class ListTagReq
    end

    class ListTagResp
      repeated :tags, Tag, 1
    end

    class RelatedTagReq
      required :tag, Tag, 1
    end

    class RelatedTagResp
      repeated :tags, Tag, 1
    end

    class ErrorResp
      required :reason, :bytes, 1
    end

    class Post
      optional :uuid, :bytes, 1
      required :title, :bytes, 2
      required :body, :bytes, 3
      repeated :tags, Tag, 4
    end

    class Tag
      required :tagname, :bytes, 1
    end
  end
end
