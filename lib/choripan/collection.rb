require 'set'

module Choripan
  class Collection
    attr_reader :posts, :logs

    def initialize
      @posts = Posts.new self
      @logs = Logs.new self
    end

    private
    class Posts
      def initialize(parent)
        @parent = parent
        @coll = Hash.new
      end

      def add(post)
        @coll[post.uuid] = post
      end

      def get(uuid)
        @coll[uuid]
      end

      def list
        @coll.values
      end
    end

    class Logs
      def initialize(parent)
        @parent = parent
        @coll = Array.new
      end

      def add(getpostreq)
        @coll << Log.new(
                         timestamp: Time.now.to_i, 
                         uuid: getpostreq.uuid,
                         signature: getpostreq.signature
                         )
      end

      def list
        @coll
      end
    end
  end
end
