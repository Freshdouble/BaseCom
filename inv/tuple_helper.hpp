#include <tuple>
#include <type_traits>

#ifndef TUPLE_HELPER_HPP__
#define TUPLE_HELPER_HPP__

namespace translib
{
    /*********************
     * To check if tuple contains a specific type.
     * From: https://stackoverflow.com/questions/25958259/how-do-i-find-out-if-a-tuple-contains-a-type
     * **/
    namespace tuple_helper
    {
        template <typename T, typename Tuple>
        struct has_type;

        template <typename T>
        struct has_type<T, std::tuple<>> : std::false_type
        {
        };

        template <typename T, typename U, typename... Ts>
        struct has_type<T, std::tuple<U, Ts...>> : has_type<T, std::tuple<Ts...>>
        {
        };

        template <typename T, typename... Ts>
        struct has_type<T, std::tuple<T, Ts...>> : std::true_type
        {
        };

        /**
         * @brief tuple_contains_type::value will return true if the tuple contains the type.
         * 
         * @tparam T 
         * @tparam Tuple 
         */
        template <typename T, typename Tuple>
        using tuple_contains_type = typename has_type<T, Tuple>::type;
    }
}
#endif