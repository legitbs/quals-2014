module Medianoche
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
        post = session.collection.posts.get self.uuid
        check_password = post.tags.detect do |t|
          t.tagname =~ /^password=/
        end

        if check_password
          desired_password = check_password.tagname
          provided_password = "password=#{self.password}"
          if (desired_password != provided_password)
            session.codec.error "Wrong password"
          end
        end

        session.codec.write :GetPostResp, post: post
      end
    end

    class ListPostReq
      def process(session)
        posts = nil
        if tag
          posts = session.collection.tags.get tag.tagname
        else
          posts = session.collection.posts.list
        end

        session.codec.write :ListPostResp, uuids: posts.map(&:uuid)
      end
    end

    class ListTagReq
      def process(session)
        tag_names = session.collection.tags.list
        tags = tag_names.map do |n|
          Tag.new tagname: n
        end

        session.codec.write :ListTagResp, tags: tags
      end
    end

    class RelatedTagReq
      def process(session)
        tag_names = session.collection.tags.related self.tag.tagname
        tags = tag_names.map do |n|
          Tag.new tagname: n
        end

        session.codec.write :RelatedTagResp, tags: tags
      end
    end
  end
end
