#ifndef UTIL_OBJPOOL_H
#define UTIL_OBJPOOL_H

#include <list>
#include <vector>

template <class T>
class ObjPool{
private:
	int size;
	std::vector<T *> chunks;
	std::list<T *> pool;
public:
	ObjPool(int init_size = 4){
		if(init_size <= 0){
			init_size = 1;
		}
		this->size = init_size;
		pre_alloc(this->size);
	}
	
	void pre_alloc(int new_size){
		T *chunk = new T[new_size];
		chunks.push_back(chunk);
		for(int i=0; i<new_size; i++){
			T *t = &chunk[i];
			pool.push_back(t);
		}
		this->size += new_size;
	}
	
	~ObjPool(){
		for(int i=0; i<chunks.size(); i++){
			delete[] chunks[i];
		}
	}
	
	T* alloc(){
		if(pool.empty()){
			pre_alloc(this->size * 2);
		}
		T *t = pool.front();
		pool.pop_front();
		return t;
	}
	
	void free(T *t){
		pool.push_front(t);
	}
};

#endif
