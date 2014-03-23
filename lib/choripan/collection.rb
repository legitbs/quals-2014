require 'set'

module Choripan
  class Collection
    attr_reader :posts

    def initialize
      @posts = Posts.new self
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
  end
end