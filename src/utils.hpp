#ifndef UTILS_HPP_INCLUDED
#define UTILS_HPP_INCLUDED

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cassert>
#include <memory>

#include <v4r/utils.hpp>

namespace v4r {

[[noreturn]] void fatalExit() noexcept;
[[noreturn]] inline void unreachable() noexcept
{
#if defined(__GNUC__) || defined(__clang__)
    __builtin_unreachable();
#else
    asm("");
#endif
}

template <typename>
struct ArraySize {};

template <typename T, size_t N>
struct ArraySize<std::array<T, N>> {
    static constexpr size_t value = N;
};

template <typename T>
class ManagedArray {
public:
    using DeleterFunc = void(void *);
    ManagedArray(T *ptr, DeleterFunc *deleter)
        : ptr_(ptr),
          deleter_(deleter)
    {}

    ManagedArray(const ManagedArray &) = delete;
    ManagedArray(ManagedArray &&o) 
        : ptr_(o.ptr_),
          deleter_(o.deleter_)
    {
        o.ptr_ = nullptr;
    }

    ~ManagedArray()
    {
        if (!ptr_) return;
        deleter_(ptr_);
    }

    constexpr const T& operator[](size_t idx) const noexcept
    {
        return ptr_[idx];
    }

    constexpr T& operator[](size_t idx) noexcept
    {
        return ptr_[idx];
    }

    T *data() { return ptr_; }
    const T *data() const { return ptr_; }

private:
    T *ptr_;
    DeleterFunc *deleter_;
};

template <typename T>
class DynArray {
public:
    DynArray(size_t num_elems)
        : data_(std::unique_ptr<T[]>(new T[num_elems])),
          num_elems_(num_elems)
    {}

    DynArray(const DynArray &) = delete;
    DynArray(DynArray &&) = default;

    constexpr const T& operator[](size_t idx) const noexcept
    {
        return data_.get()[idx];
    }

    constexpr T& operator[](size_t idx) noexcept
    {
        return data_.get()[idx];
    }

    T *data() { return data_.get(); }
    const T *data() const { return data_.get(); }

    constexpr size_t size() const noexcept { return num_elems_; }

    template <typename U>
    class IterBase {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T *;
        using reference = T &;
        
        explicit IterBase(T *ptr) : ptr_(ptr) {}

        IterBase& operator+=(difference_type d)
        {
            ptr_ += d;
            return *this;
        }

        IterBase& operator-=(difference_type d)
        {
            ptr_ -= d;
            return *this;
        }

        friend IterBase operator+(IterBase it, difference_type d)
        {
            it += d;
            return it;
        }

        friend IterBase operator+(difference_type d, IterBase it)
        {
            return it + d;
        }

        friend IterBase operator-(IterBase it, difference_type d)
        {
            it -= d;
            return it;
        }

        friend difference_type operator-(const IterBase &a,
                                         const IterBase &b)
        {
            return a.ptr_ - b.ptr_;
        }

        bool operator==(IterBase o) const { return ptr_ == o.ptr_; }
        bool operator!=(IterBase o) const { return !(*this == o); }

        reference operator[](difference_type d) const { return ptr_[d]; }
        reference operator*() const { return *ptr_; }

        friend bool operator<(const IterBase &a, const IterBase &b)
        {
            return a.ptr_ < b.ptr_;
        }

        friend bool operator>(const IterBase &a, const IterBase &b)
        {
            return a.ptr_ > b.ptr_;
        }

        friend bool operator<=(const IterBase &a, const IterBase &b)
        {
            return !(a > b);
        }

        friend bool operator>=(const IterBase &a, const IterBase &b)
        {
            return !(a < b);
        }

        IterBase &operator++() { *this += 1; return *this; };
        IterBase &operator--() { *this -= 1; return *this; };

        IterBase operator++(int)
        {
            IterBase t = *this;
            operator++();
            return t;
        }
        IterBase operator--(int)
        {
            IterBase t = *this;
            operator--();
            return t;
        }
    private:
        T *ptr_;
    };

    using iterator = IterBase<T>;
    using const_iterator = IterBase<const T>;

    iterator begin() { return iterator(data_.get()); }
    iterator end() { return iterator(data_.get() + num_elems_); }

    const_iterator begin() const { return const_iterator(data_.get()); }
    const_iterator end() const { return const_iterator(data_.get() + num_elems_); }

private:
    std::unique_ptr<T[]> data_;
    size_t num_elems_;
};

template <typename T>
class StridedSpan {
public:
    using RawPtrType = std::conditional_t<std::is_const_v<T>,
                                          const uint8_t *,
                                          uint8_t *>; 

    StridedSpan(RawPtrType data, size_t num_elems, size_t byte_stride)
        : raw_data_(data),
          num_elems_(num_elems),
          byte_stride_(byte_stride)
    {}

    constexpr const T& operator[](size_t idx) const noexcept
    {
        return *fromRaw(raw_data_ + idx * byte_stride_);
    }

    constexpr T& operator[](size_t idx) noexcept
    {
        return *fromRaw(raw_data_ + idx * byte_stride_);
    }

    T *data() { return fromRaw(raw_data_); }
    const T *data() const { return fromRaw(raw_data_); }

    constexpr size_t size() const noexcept { return num_elems_; }

    template <typename U>
    class IterBase {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = U;
        using difference_type = std::ptrdiff_t;
        using pointer = U *;
        using reference = U &;

        IterBase(RawPtrType ptr, size_t byte_stride)
            : ptr_(ptr),
              byte_stride_(byte_stride)
        {}

        IterBase& operator+=(difference_type d)
        {
            ptr_ += d * byte_stride_;
            return *this;
        }

        IterBase& operator-=(difference_type d)
        {
            ptr_ -= d * byte_stride_;
            return *this;
        }

        friend IterBase operator+(IterBase it, difference_type d)
        {
            it += d;
            return it;
        }

        friend IterBase operator+(difference_type d, IterBase it)
        {
            return it + d;
        }

        friend IterBase operator-(IterBase it, difference_type d)
        {
            it -= d;
            return it;
        }

        friend difference_type operator-(const IterBase &a,
                                         const IterBase &b)
        {
            assert(a.byte_stride_ == b.byte_stride_);
            return (a.ptr_ - b.ptr_) / a.byte_stride_;
        }

        bool operator==(IterBase o) const { return ptr_ == o.ptr_; }
        bool operator!=(IterBase o) const { return !(*this == o); }

        reference operator[](difference_type d) const
        {
            return *(*this + d);
        }
        reference operator*() const
        {
            return *fromRaw(ptr_);
        }

        friend bool operator<(const IterBase &a, const IterBase &b)
        {
            return a.ptr_ < b.ptr_;
        }

        friend bool operator>(const IterBase &a, const IterBase &b)
        {
            return a.ptr_ > b.ptr_;
        }

        friend bool operator<=(const IterBase &a, const IterBase &b)
        {
            return !(a > b);
        }

        friend bool operator>=(const IterBase &a, const IterBase &b)
        {
            return !(a < b);
        }

        IterBase &operator++() { *this += 1; return *this; };
        IterBase &operator--() { *this -= 1; return *this; };

        IterBase operator++(int)
        {
            IterBase t = *this;
            operator++();
            return t;
        }
        IterBase operator--(int)
        {
            IterBase t = *this;
            operator--();
            return t;
        }
    private:
        RawPtrType ptr_;
        size_t byte_stride_;
    };

    using iterator = IterBase<T>;
    using const_iterator = IterBase<const T>;

    iterator begin()
    {
        return iterator(raw_data_, byte_stride_);
    }
    iterator end()
    {
        return iterator(raw_data_ + num_elems_ * byte_stride_, byte_stride_);
    }

    const_iterator begin() const
    {
        return const_iterator(raw_data_, byte_stride_);
    }
    const_iterator end() const
    {
        return const_iterator(raw_data_ + num_elems_ * byte_stride_,
                              byte_stride_);
    }

    bool contiguous() const { return byte_stride_ == value_size_; }

private:
    RawPtrType raw_data_;
    size_t num_elems_;
    size_t byte_stride_;

    static constexpr size_t value_size_ = sizeof(T);

    static RawPtrType toRaw(T *ptr)
    {
        if constexpr (std::is_same_v<T *, RawPtrType>) {
            return ptr;
        } else {
            return reinterpret_cast<RawPtrType>(ptr);
        }
    }

    static T * fromRaw(RawPtrType ptr)
    {
        if constexpr (std::is_same_v<T *, RawPtrType>) {
            return ptr;
        } else {
            return reinterpret_cast<T *>(ptr);
        }
    }

    friend class IterBase<T>;
};

template <typename T, typename... Args>
inline Handle<T> make_handle(Args&&... args)
{
    return Handle<T>(new T(std::forward<Args>(args)...));
};

template <typename T>
inline void HandleDeleter<T>::operator()(std::remove_extent_t<T> *ptr) const
{
    if constexpr (std::is_array_v<T>) {
        delete[] ptr;
    } else {
        delete ptr;
    }
}

}

#endif
