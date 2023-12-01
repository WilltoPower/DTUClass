/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtulog.h
  *Description:
    定义动态缓存
  *History:
    1, 创建, wangjs, 2021-7-16
        2, 添加一个set的接口， wangjs, 2021-7-23
        3, 添加移动构造函数, wangjs, 2021-9-26
**********************************************************************************/
#ifndef _DTU_BUFFER_H
#define _DTU_BUFFER_H
#include <string>
#include <sstream>
#include <vector>
#ifndef _WIN32
#include <string.h>
#endif
#ifdef _WIN32
#include <msgpack.hpp>
#else
#include <msgpack.hpp>
#endif
namespace DTU {
class buffer {
public:
    buffer();
    buffer(const uint32_t size);
    buffer(const char *src, size_t size);
    buffer(const buffer &src);
    buffer(buffer &&src);
    ~buffer();

public:
    // overload some operator
    buffer &operator=(const buffer &src);

    /*
    @function : joint buffer
    */
    buffer &operator+(const buffer &src);

public:
    // attribute fuctions
    /*
    @function : return current size;
    */
    uint32_t size() const;

    /*
    @function : resize buffer
    */
    void resize(uint32_t size);
    /*
    @function : get vector data;
    */
    const char *const_data() const;

    char *data();
    /*
    @function : get different value from buffer;
    only used in base data type;
    */
    template <typename T, typename = std::enable_if<!std::is_same<T, std::string>::value>> T value() {
        if (sizeof(T) > _buffer.size() || _buffer.empty()) {
            std::stringstream ss;
            ss << std::string("DTU::buffer::value invalid type, size:") << sizeof(T) << std::string(" buffer size:")
               << _buffer.size();
            throw std::runtime_error(ss.str().c_str());
        }
        if (std::is_same<T, std::string>::value) {

            // return (T)std::string(_buffer.data());
        }
			// if constexpr (std::is_same<T, std::string>::value){
			// 	return std::string(_buffer.data());
			// }
        T v;
#ifdef _WIN32
        memcpy_s(&v, sizeof(T), _buffer.data(), sizeof(T));
#else
        memcpy(&v, _buffer.data(), sizeof(T));
#endif
        return std::move(v);
    }

public:
    /*
    @function : replace data at pos with src;
    @param pos : the begin position to set data;
    @param src : replace data;
    */
    void set(uint32_t pos, const char *src, uint32_t srcsize);

    /*
    @function : replace data at pos with src;
    @param pos : the begin position to set data;
    @param src : replace data;
    */
    void set(uint32_t pos, const buffer &src);

    /*
    @function : get data from buffer at position
    @param pos : destination position;
    @param size : destination size;
    @return : new buffer
    */
    buffer get(uint32_t pos, uint32_t size) const;

    /*
    @function : look over data at pos
    @param pos : destination position;
    @param size : destination size;
    @return : data pointer at pos;
    */
    const char *query(uint32_t pos, uint32_t size) const;

    /*
    @function : append data at the end of buffer
    @param src : data pointer to append;
    @param size : data size;
    @return : buffer after append;
    */
    buffer &append(const char *src, uint32_t size);

    /*
    @function : append data at the end of buffer
    @param src : data to append;
    @return : buffer after append;
    */
    buffer &append(const buffer &src);

    /*
    @function : insert data at pos
    @param pos : insert position
    @param src : src data to insert
    */
    void insert(uint32_t pos, const buffer &src);

    /*
    @function : insert data at pos
    @param pos : insert position
    @param src : src data to insert
    @param size : src data length
    */
    void insert(uint32_t pos, const char *src, uint32_t size);

    /*
    @function : remove data
    @param pos : remove begin position
    @param size : remove size
    */
    void remove(uint32_t pos, uint32_t size);

    /*
    @function : remove all data
    */
    void remove();

    /*
    @function : check buffer is empty
    */
    bool empty();

    //??
    void dump(uint32_t pos, uint32_t size) const {

        // THROW_RUNTIME_ERROR_IF(pos > _buffer.size() || pos+size>_buffer.size(), "DVBuff Dump invalid position");
        int count = 0;
        for (size_t i = pos; i < pos + size; i++) {
            if (count % 32 == 0){
                printf("\n");
            }
            printf("0x%02X ", _buffer[i]);
            count++;
        }
        printf("\n");
    }

    std::string to_string() { return std::string(_buffer.data(), _buffer.size()); }

    MSGPACK_DEFINE(_buffer)
    //MSGPACK_DEFINE_ARRAY(_buffer)
private:
    std::vector<char> _buffer;
};
}; // namespace DTU
#endif
