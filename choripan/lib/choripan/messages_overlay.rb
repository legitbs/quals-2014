module Choripan
  module Messages
    class StorePostReq
      def process(session)
        self.post.uuid ||= SecureRandom.uuid
        session.collection.posts.add self.post
        session.codec.write :StorePostResp, uuid: self.post.uuid
      end
    end

    class GetPostReq
      def process(session)
        begin
          valid = session.signer.verify self.uuid, self.signature
        rescue => e
          session.codec.write :ErrorResp, reason: e
          return
        end
        unless valid
          session.codec.write :ErrorResp, reason: "invalid signature"
          return
        end
        
        post = session.collection.posts.get self.uuid

        session.codec.write :GetPostResp, post: post
      end
    end

    class ListPostReq
      def process(session)
        posts = session.collection.posts.list

        session.codec.write :ListPostResp, uuids: posts.map(&:uuid)
      end
    end

    class ListLogReq
      def process(session)
        logs = session.collection.logs.list
pp logs
        session.codec.write :ListLogResp, logs: logs
      end
    end
  end
end
