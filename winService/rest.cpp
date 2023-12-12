#include "rest.hpp"


using namespace rest;

restAPI::restAPI(){
    this->db = kuzu_database_init("C:\\kuzu-test", kuzu_default_system_config());
    this->conn = kuzu_connection_init(db);
}

std::string restAPI::get(){
    kuzu_query_result* result = kuzu_connection_query(this->conn, "MATCH (a:Person) RETURN a.name AS NAME, a.age AS AGE;");
    std::string data = "";
    while (kuzu_query_result_has_next(result)) {
        kuzu_flat_tuple* tuple = kuzu_query_result_get_next(result);
        kuzu_value* value = kuzu_flat_tuple_get_value(tuple, 0);
        char* name = kuzu_value_get_string(value);
        value = kuzu_flat_tuple_get_value(tuple, 1);
        int64_t age = kuzu_value_get_int64(value);
        kuzu_value_destroy(value);
        data += std::to_string(age);
        data += " ";
        data += name;
        data += "\n";
        kuzu_flat_tuple_destroy(tuple);
    }
    return data;
}

void restAPI::put(const http_request& req) {
    std::string_view name1 = req.get_arg("name");
    std::string_view age1 = req.get_arg("age");
    std::string name(name1);
    std::string ageString(age1);

    std::string query = "CREATE (p:Person {name: '" + name + "', age: " + ageString + "})";

    kuzu_query_result* result = kuzu_connection_query(conn, query.c_str());
}


void restAPI::remove(const http_request& req){
    std::string_view name1 = req.get_arg("name");
    std::string name(name1);

    std::string query = "MATCH (u:Person) WHERE u.name = '"+ name +"' DELETE u RETURN u.*;";

    kuzu_query_result* result = kuzu_connection_query(conn, query.c_str());
}

void restAPI::update(const http_request& req){
    std::string_view name1 = req.get_arg("name");
    std::string name(name1);
    std::string_view uname1 = req.get_arg("uname");
    std::string uname(uname1);

    std::string query = "MATCH (u:Person) WHERE u.name = '"+ name +"' SET u.age = " + uname + ";";

    kuzu_query_result* result = kuzu_connection_query(conn, query.c_str());
}