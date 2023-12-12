#ifndef INCLUDED_REST
#define INCLUDED_REST

#include "httpserver.hpp"
#include "kuzu.h"

using namespace httpserver;
//namespace controller
namespace rest
{
    class restAPI
    {
        public:
            restAPI();
            std::string get();
            void put(const http_request& req);
            void remove(const http_request& req);
            void update(const http_request& req);

        private:
            kuzu_database* db;
            kuzu_connection* conn;
    };

} // namespace rest

#endif