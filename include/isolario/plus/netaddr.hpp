#ifndef ISOLARIO_PLUS_NETADDR_HPP_
#define ISOLARIO_PLUS_NETADDR_HPP_

extern "C" {
#include <isolario/netaddr.h>
#include <isolario/endian.h>
//#include <isolario/u128_t.h>
}

#include <cstdint>
#include <string>

namespace isocore {
    
    enum {
        IPv4_BYTE_SIZE = sizeof(struct in_addr),
        IPv4_BIT_SIZE = IPv4_BYTE_SIZE * 8,
        IPv6_BYTE_SIZE = sizeof(struct in6_addr),
        IPv6_BIT_SIZE = IPv6_BYTE_SIZE * 8,
    };
    
    enum {
        AFI_IPv4 = 1,
        AFI_IPv6 = 2,
        SAFI_UNICAST = 1,
        AF_INVALID = AF_UNSPEC,
    };
    
    inline constexpr uint8_t bit_to_byte_len(uint8_t bitlen) noexcept
    {
        return (bitlen >> 3) + ((bitlen & 7) != 0);
    }
    
    class netaddr {
    public:        
        netaddr()
        {
            addr.bitlen = 0;
            addr.family = AF_UNSPEC;
            memset(&addr.sin6, 0, sizeof(addr.sin6));
        }
        
        netaddr(const char* ip) noexcept
        {
            ::stonaddr(&addr, ip);
        }

        netaddr(const uint8_t *ip_raw, bool ipv6)
        {
            if (!ipv6) {
                ::makenaddr(&addr, ip_raw + 1, ip_raw[0]);
            } else {
                ::makenaddr6(&addr, ip_raw + 1, ip_raw[0]);
            }
        }

        netaddr(const netaddr&) noexcept = default;

        netaddr& operator=(const netaddr&) noexcept = default;
        
        /*netaddr& operator++()
        {
            if (this->get_family() == AF_INET) {
                uint32_t *ptr = &(this->addr.u32[0]);
                uint32_t val = frombig32(*ptr);
                *ptr = htonl(++val);
            } else {
                uint64_t *ptr = reinterpret_cast<uint64_t*>(&this->addr.sin6);
                uint64_t upper = frombig64(*ptr++);
                uint64_t lower = frombig64(*ptr--);
                u128_t num = u128from(upper, lower);
                num = u128addu(num, 1);
                *ptr = tobig64(u128upper(num));
                ptr++;
                *ptr = tobig64(u128lower(num));
            }
            return *this;
        }
        
        netaddr operator++(int)
        {
            return this->operator++();
        }*/

        operator netaddr_t&() noexcept { return addr; }
        operator const netaddr_t&() const noexcept { return addr; }

        static int naddrsize(int bitlen) noexcept
        {
            return ::naddrsize(bitlen);
        }

        static std::uint8_t get_family(const char *ip) noexcept
        {
            return ::saddrfamily(ip);
        }

        std::uint8_t* raw_ptr() noexcept { return addr.bytes; }
        const std::uint8_t* raw_ptr() const noexcept { return addr.bytes; }
        
        uint8_t get_family() const noexcept { return addr.family; }
        
        uint8_t get_bitlen() const noexcept { return addr.bitlen; }
        void set_bitlen(uint8_t val) { addr.bitlen = val; }

        const std::string to_string(bool with_mask = true) const
        {
            int mode = (with_mask) ? NADDR_CIDR : NADDR_PLAIN;
            return ::naddrtos(&addr, mode);
        }
        
        bool is_reserved() const noexcept { return ::isnaddrreserved(&addr); }
        
        bool is_valid() const noexcept { return addr.family != AF_UNSPEC; }
        
        bool includes(netaddr &p2) const noexcept
        {
            if (this->get_family() != p2.get_family())

            if (this->get_bitlen() > p2.get_bitlen())
                return false;

            unsigned int bytes = this->get_bitlen() >> 3;
            unsigned int bits = this->get_bitlen() & 3;
            if (memcmp(this->raw_ptr(), p2.raw_ptr(), bytes) != 0)
                return 0;

            uint8_t b = uint8_t(0xff << (8 - bits));
            return bits == 0 || (this->raw_ptr()[bytes] & b) == (p2.raw_ptr()[bytes] & b);
        }
        
        /*u128_t cov()
        {
        
        }*/
    private:
        netaddr_t addr;

        friend bool operator==(const netaddr& a, const netaddr& b) noexcept;
        friend bool operator<(const netaddr& a, const netaddr& b) noexcept;
        friend bool comp_with_mask(const netaddr &src, const netaddr &dest, int mask) noexcept;
    };

    inline bool operator==(const netaddr& a, const netaddr& b) noexcept
    {
        return ::prefixeq(&a.addr, &b.addr);
    }

    inline bool operator!=(const netaddr& a, const netaddr& b) noexcept
    {
        return !(a == b);
    }
    
    inline bool operator<(const netaddr& a, const netaddr& b) noexcept
    {
        if (a.get_family() != b.get_family())
            return a.get_family() < b.get_family();

        int maxbitlen = (a.get_bitlen() > b.get_bitlen()) ? a.get_bitlen() : b.get_bitlen();
        
        int res = memcmp(a.raw_ptr(), b.raw_ptr(), ::naddrsize(maxbitlen));
        
        return (res == 0) ? a.get_bitlen() < b.get_bitlen() : res < 0;
    }
    
    inline bool comp_with_mask(const netaddr &src, const netaddr &dest, int mask) noexcept
    {
        return ::prefixeqwithmask(&src.addr, &dest.addr, mask);
    }

    inline std::string expand_IPv6_str(const std::string &ip)
    {
        struct in6_addr dst;
        if (inet_pton(AF_INET6, ip.c_str(), (void *)&dst) <= 0)
            return "";
        
        char res[INET6_ADDRSTRLEN + 1]; 
        sprintf(res, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x", // '\0' is added by sprintf
            (int)dst.s6_addr[0], (int)dst.s6_addr[1],
            (int)dst.s6_addr[2], (int)dst.s6_addr[3],
            (int)dst.s6_addr[4], (int)dst.s6_addr[5],
            (int)dst.s6_addr[6], (int)dst.s6_addr[7],
            (int)dst.s6_addr[8], (int)dst.s6_addr[9],
            (int)dst.s6_addr[10], (int)dst.s6_addr[11],
            (int)dst.s6_addr[12], (int)dst.s6_addr[13],
            (int)dst.s6_addr[14], (int)dst.s6_addr[15]);
        
        return std::string(res);
    }
    
    inline std::string compress_IPv6_str(const std::string &ip) // TODO: optimize if necessary
    {
        struct in6_addr add;
        if (inet_pton(AF_INET6, ip.c_str(), &add) != 1)
            return "";
            
        char res[INET6_ADDRSTRLEN + 1];
        return inet_ntop(AF_INET6, &add, res, sizeof(res));
    }
    
    class netaddrap {
    public:

        netaddrap(const netaddrap&) noexcept = default;

        netaddrap& operator=(const netaddrap&) noexcept = default;

        operator netaddr&() noexcept { return *reinterpret_cast<netaddr*>(this); }
        operator const netaddr&() const noexcept { return *reinterpret_cast<const netaddr*>(this); }

        operator netaddrap_t&() noexcept { return addr; }
        operator const netaddrap_t&() const noexcept { return addr; }

    private:
        netaddrap_t addr;
    };
}

#endif
