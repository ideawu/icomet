#ifndef ICOMET_OBJPOOL_H
#define ICOMET_OBJPOOL_H

#include <list>
#include <vector>

template <class T>
class ObjPool{
private:
	T *used;
	std::vector<T> buf;
	std::list<T *> pool;
public:
	ObjPool(){
		used = NULL;
		resize(4);
	}
	
	void resize(int new_size){
		int old_size = buf.size();
		buf.resize(new_size);
		for(int i=old_size; i<new_size; i++){
			T *t = &buf[i];
			pool.push_back(t);
		}
	}
	
	~ObjPool(){
	}
	
	T* alloc(){
		if(pool.empty()){
			resize(buf.size() * 2);
		}
		T *t = pool.front();
		pool.pop_front();
		return t;
	}
	
	void free(T *t){
		pool.push_back(t);
	}
};

#endif
