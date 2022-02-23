//
// Created by l1nkkk on 3/23/21.
//

#ifndef LOGGER_SINGLETON_H
#define LOGGER_SINGLETON_H

namespace l1nkkk{
/**
 *
 * @tparam T    类型
 * @tparam x    为了创造多个实例对应的Tag
 * @tparam N    同一个Tag创造多个实例索引
 */
template<typename T, typename  x = void, int N = 0>
class Singleton{
public:
    static T* GetInstance(){
        static T v;
        return &v;
    }
};
}

#endif //LOGGER_SINGLETON_H
