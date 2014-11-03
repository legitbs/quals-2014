module Choripan
  class Packer
    def unpack(str)
      r_buf = ''
      s_buf = ''
      str.bytes.each_slice(2) do |slice|
        a, b = slice
        r_buf << (a).chr
        s_buf << (b).chr
      end

      [r_buf.to_i(36), s_buf.to_i(36)]
    end

    def pack(r, s)
      r_p = r.to_s(36)
      s_p = s.to_s(36)

      r_p, s_p = pad(r_p, s_p)

      r_p.bytes.zip(s_p.bytes).flatten.map(&:chr).join
    end

    private
    def pad(str1, str2)
      l1 = str1.length
      l2 = str2.length

      return [str1, str2] if l1 == l2
      
      diff = (l1 - l2).abs

      if l1 > l2
        return [str1, ('0' * diff) + str2]
      else
        return [('0' * diff) + str1, str2]
      end
    end
  end
end
