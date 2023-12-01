/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtulog.h
  *Description: 
    定义动态缓存
  *History: 
    1, 创建, wangjs, 2021-7-16
    2，实现所有的接口， wangjs, 2021-7-19
    3, 修改追加和插入功能，能够直接向空缓存操作， wangjs, 2021-7-23
    4, 调整了赋值号=, 减少内存分配操作, wangjs, 2021-8-4
    5, 修改为vector<char>实现，提供给msgpackage使用, wangjs, 2021-8-4
**********************************************************************************/
#include "dtubuffer.h"
#include <dtuerror.h>
#include <string.h>
using namespace DTU;

#define DTU_CALLOC(size) \
    (char*)calloc(1, size);

void* dtu_realloc(char* ptr, uint32_t size, uint32_t destsize){
    if (destsize <= size){
        return ptr;
    }
    char* newptr = DTU_CALLOC(destsize);
#ifdef _WIN32
    memcpy_s(newptr, size, ptr, size);
#else
    memcpy(newptr, ptr, size);
#endif
    if (ptr){
        free(ptr);
    }
    ptr = newptr;
    return ptr;
}

buffer::buffer(){

}
buffer::~buffer(){
    //destory();
}

buffer::buffer(const uint32_t size){
    _buffer.resize(size);
}

buffer::buffer(const char* src, size_t size)
{
    DTU_USER();
    if (src == nullptr || size == 0){
        DTU_THROW((char*)"buffer::buffer 错误的输入参数:地址%p, %u", src, size);
    }
    _buffer.clear();
    _buffer.resize(size);
#ifdef _WIN32
    memcpy_s(_buffer.data(), size, src, size);
#else
    memcpy(_buffer.data(), src, size);
#endif
}

buffer::buffer(const buffer& src)
{
    _buffer = src._buffer;
}

buffer::buffer(buffer&& src)
{
    _buffer = src._buffer;
}

buffer& buffer::operator=(const buffer& src)
{
    _buffer = src._buffer;
    return *this;
}

buffer& buffer::operator+(const buffer& src)
{
    _buffer.reserve(_buffer.size() + src.size());
	_buffer.insert(_buffer.end(), src._buffer.begin(), src._buffer.end());
    return *this;
}

uint32_t buffer::size() const 
{
    return (uint32_t)_buffer.size();
}

void buffer::resize(uint32_t size)
{
    _buffer.resize(size);
}

const char* buffer::const_data() const{
    return _buffer.data();
}

void buffer::set(uint32_t pos, const char* src, uint32_t srcsize)
{
    if (src == nullptr){
        return;
    }
    DTU_USER()
    if (srcsize + pos > _buffer.size()){
        DTU_THROW((char*)"buffer::set() 1 源长度大于当前有效长度:%u, %u, 有效长度:%u",
             pos, srcsize, _buffer.size());
    }
    #ifdef _WIN32
        memcpy_s(_buffer.data()+pos, srcsize, src, srcsize);
    #else
        memcpy(_buffer.data()+pos, src,srcsize);
    #endif
}

void buffer::set(uint32_t pos, const buffer& src)
{
    DTU_USER()
    if (src.size()+pos > _buffer.size()){
        DTU_THROW((char*)"buffer::set() 2 源长度大于当前有效长度:%u, %u, 有效长度:%u",
             pos, src.size(), _buffer.size());
    }
    #ifdef _WIN32
        memcpy_s(_buffer.data()+pos, src.size(), src.const_data(), src.size());
    #else
        memcpy(_buffer.data()+pos, src.const_data(),src.size());
    #endif
}

buffer buffer::get(uint32_t pos, uint32_t size) const
{
    DTU_USER()
    if (size + pos > _buffer.size()){
        DTU_THROW((char*)"buffer::get() 1 目标长度大于当前有效长度:位置%u, 获取长度%u, 有效长度:%u",
             pos, size, _buffer.size());
    }
    buffer value(_buffer.data()+pos, size);
    return std::move(value);
}

const char* buffer::query(uint32_t pos, uint32_t size) const
{
    DTU_USER()
    if (size + pos > _buffer.size()){
        DTU_THROW((char*)"buffer::get() 2 目标长度大于当前有效长度:位置%u, 获取长度%u, 有效长度:%u",
             pos, size, _buffer.size());
    }
    return _buffer.data()+pos;
}

buffer& buffer::append(const char* src, uint32_t size)
{
    _buffer.reserve(_buffer.size() + size);
	_buffer.insert(_buffer.end(), src,src+size);
    return *this;
}

buffer& buffer::append(const buffer& src)
{
    _buffer.reserve(_buffer.size() + src.size());
	_buffer.insert(_buffer.end(), src.const_data(), src.const_data() + src.size());
    return *this;
}

void buffer::insert(uint32_t pos, const buffer& src)
{
	if (pos>=_buffer.size())
    {
		this->append(src);
	}
	else 
    {
		_buffer.insert(_buffer.begin()+pos, src.const_data(), src.const_data()+src.size());
	}
}

void buffer::insert(uint32_t pos, const char* src, uint32_t size)
{
	if (pos >= _buffer.size()){
		this->append(src,size);
	} 
	else{
		_buffer.insert(_buffer.begin()+pos, src, src+size);
	}
}

void buffer::remove(uint32_t pos, uint32_t size)
{
    // 不删除
	if (size == 0){
		return;
	}
    DTU_USER()
    if (pos + size > _buffer.size()){
        DTU_THROW((char*)"buffer::remove 错误的参数:pos:%u, size:%u, _size:%u",
            pos, size, _buffer.size());
    }
    _buffer.erase(_buffer.begin()+pos, _buffer.begin() + pos + size);
}

void buffer::remove()
{
    _buffer.clear();
}

bool buffer::empty(){
    return _buffer.empty();
}

char* DTU::buffer::data()
{
	return _buffer.data();
}
