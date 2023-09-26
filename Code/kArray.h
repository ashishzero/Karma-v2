#include "kContext.h"
#include <string.h>

template <typename T>
struct kArray {
	imem  count;
	imem  allocated;
	T    *data;

	kArray(): count(0), data(nullptr), allocated(0) {}
	operator kSlice<T>()             { return kSlice<T>(data, count); }
	operator const kSlice<T>()       const { return kSlice<T>(data, count); }
	T &operator[](ptrdiff_t i)       { kAssert(i >= 0 && i < count); return data[i]; }
	const T &operator[](ptrdiff_t i) const { kAssert(i >= 0 && i < count); return data[i]; }
	T *      begin()                 { return data; }
	T *      end()                   { return data + count; }
	const T *begin() const           { return data; }
	const T *end() const             { return data + count; }

	imem NextCapacity(imem count) {
		imem new_cap = allocated ? (allocated << 1) : 8;
		return new_cap > count ? new_cap : count;
	}

	T *TryGet(imem index) {
		if (index < count) {
			return &data[index];
		}
		return 0;
	}

	T &First(void) {
		kAssert(count != 0);
		return data[0];
	}

	T &Last(void) {
		kAssert(count != 0);
		return data[count - 1];
	}

	const T &First(void) const {
		kAssert(count != 0);
		return data[0];
	}

	const T &Last(void) const {
		kAssert(count != 0);
		return data[count - 1];
	}

	void Pop(void) {
		kAssert(count > 0);
		count -= 1;
	}

	void Reset(void) {
		count = 0;
	}

	void Remove(imem index) {
		kAssert(index < count);
		memmove(data + index, data + (index + 1), (count - index - 1) * sizeof(T));
		count -= 1;
	}

	void RemoveUnordered(imem index) {
		kAssert(index < count);
		data[index] = data[count - 1];
		count -= 1;
	}

	bool Reserve(imem req_cap) {
		if (req_cap <= allocated)
			return true;

		void *mem = kRealloc(data, allocated * sizeof(T), req_cap * sizeof(T));
		if (mem) {
			data      = (T *)mem;
			allocated = req_cap;
			return true;
		}

		return false;
	}

	bool Resize(imem n) {
		if (Reserve(n)) {
			count = n;
			return true;
		}
		return false;
	}

	bool Resize(imem n, const T &src) {
		if (Reserve(n)) {
			for (imem idx = count; idx < n; ++idx) {
				data[idx] = src;
			}
			count = n;
			return true;
		}
		return false;
	}

	T *Extend(imem n) {
		imem req_count = count + n;
		imem cur_count = count;

		if (req_count < allocated) {
			count = req_count;
			return &data[cur_count];
		}

		imem new_cap = NextCapacity(req_count);

		if (Reserve(new_cap)) {
			count = req_count;
			return &data[cur_count];
		}

		return 0;
	}

	T *Add(void) {
		return Extend(1);
	}

	void Add(const T &src) {
		T *dst = Add();
		if (dst)
			*dst = src;
	}

	bool CopyBuffer(T *src, imem src_count) {
		if (count + src_count > allocated) {
			imem new_cap = NextCapacity(allocated + src_count);
			if (!Reserve(new_cap))
				return false;
		}

		memcpy(data + count, src, src_count * sizeof(T));
		count += src_count;

		return true;
	}

	bool CopyArray(kSlice<T> src) {
		if (src.count) {
			return CopyBuffer(src.data, src.count);
		}
		return false;
	}

	bool Insert(imem index, const T &src) {
		kAssert(index < count);

		if (Add()) {
			memmove(data + (index + 1), data + index, (count - index));
			data[index] = src;
			return true;
		}

		return false;
	}

	void Pack(void) {
		if (count != allocated) {
			void *mem = kRealloc(data, allocated * sizeof(T), count * sizeof(T));
			if (mem) {
				data      = (T *)mem;
				allocated = count;
			}
		}
	}

	kArray<T> Clone(void) {
		kArray<T> dst = {};
		dst.CopyBuffer(data, count);
		return dst;
	}
};

template <typename T>
void kFree(kArray<T> *arr) {
	kFree(arr->data, arr->allocated * sizeof(T));
}

template <typename T>
void kFree(kSlice<T> *arr) {
	kFree(arr->data, arr->count * sizeof(T));
}

//
//
//

template <typename T, typename SearchFunc, typename... Args>
imem Find(kSlice<T> arr, SearchFunc func, const Args &...args) {
	for (imem index = 0; index < arr.count; ++index) {
		if (func(arr[index], args...)) {
			return index;
		}
	}
	return -1;
}

template <typename T>
imem Find(kSlice<T> arr, const T &v) {
	for (imem index = 0; index < arr.count; ++index) {
		auto elem = arr.data + index;
		if (*elem == v) {
			return index;
		}
	}
	return -1;
}
