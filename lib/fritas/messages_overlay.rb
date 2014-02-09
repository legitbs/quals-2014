module Fritas
  module Messages
    class StorePostReq
      def process(session)
        self.post.uuid ||= SecureRandom.uuid
        session.collection.posts.add self.post
        session.codec.send :StorePostResp, uuid: self.post.uuid
      end
    end

    class GetPostReq
      def process(session)
        post = session.collection.posts.get self.uuid

        session.codec.send :GetPostResp, post
      end
    end
  end
end