// main.cpp

#include <boost/json/object.hpp>
#include <cstdlib>
#include <iostream>
#include <pqxx/pqxx>

#include <boost/json/src.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/value.hpp>

#include <optional>

namespace json = boost::json;

using namespace std::literals;
// libpqxx использует zero-terminated символьные литералы вроде "abc"_zv;
using pqxx::operator"" _zv;

constexpr auto tag_add_book = "add_book"_zv;
constexpr auto tag_all_books = "all_books"_zv;

struct Book {
    std::string title;
    std::string author;
    int year;
    std::string ISBN;
};

struct Command {
    std::string action;
    json::value payload;
};

struct Result {
    std::string res;
};

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, Result const& res) {
    json::object object;
    object["result"] = res.res;
    jv.emplace_object() = object;
}

Command ReadCommand(std::string str) {
    // Распарсить строку как JSON, используя boost::json::parse
    // Получаем json-объект из строки (тип value)
    auto parsed_config_json = json::parse(str);
    auto obj = parsed_config_json.as_object();

    return {
        value_to<std::string>(obj.at(std::string("action"))),
        obj.at(std::string("payload"))
    };
}

Book ReadBookFromJsonValue(const json::value& val) {
    auto obj = val.as_object();

    auto ISBN = obj.at(std::string("ISBN"));

    std::string IBSN_res;
    if(ISBN.is_string()) {
        IBSN_res = ISBN.as_string();
    } else {
        IBSN_res = "null";
    }

    return {
        value_to<std::string>(obj.at(std::string("title"))),
        value_to<std::string>(obj.at(std::string("author"))),
        value_to<int>(obj.at(std::string("year"))),
        IBSN_res
    };
}

json::object AddBook(pqxx::connection& conn, const json::value& payload) {
    json::object res;
    Book book;
    try {
        book = ReadBookFromJsonValue(payload);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        res["result"] = "false";
        return res;
    }

    try {
        // Создаём транзакцию.
        pqxx::work w(conn);
        std::optional<std::string> ISBN = std::nullopt;
        if (book.ISBN != "null") {
             ISBN = book.ISBN;
        }
        w.exec_prepared(tag_add_book, book.title, book.author, book.year, ISBN);
        w.commit();
    } catch (const std::exception& e) {
        res["result"] = "false";
        return res;
    }

    res["result"] = "true";
    return res;
}

json::array AllBooks(pqxx::connection& conn) {
    auto query_text = "SELECT * FROM books ORDER BY year DESC, title, author, ISBN"_zv;
    // Создаём транзакцию чтения.
    pqxx::read_transaction r(conn);

    json::array json_array;

    // Выполняем запрос и итерируемся по строкам ответа
    for (auto [id, title, author, year, ISBN] 
        : r.query<int, std::string_view, std::string_view, int, std::optional<std::string>>(query_text)) 
    {
        json::object json_object;
        json_object["id"] = id;
        json_object["title"] = title;
        json_object["author"] = author;
        json_object["year"] = year;
        json_object["ISBN"];
        if (ISBN) {
            json_object["ISBN"] = *ISBN;
        }   
        json_array.push_back(json_object);
    }

    return json_array;
}

int main(int argc, const char* argv[]) {
    try {
        if (argc == 1) {
            std::cout << "Usage: db_example <conn-string>\n"sv;
            return EXIT_SUCCESS;
        } else if (argc != 2) {
            std::cerr << "Invalid command line\n"sv;
            return EXIT_FAILURE;
        }

        // Подключаемся к БД, указывая её параметры в качестве аргумента
        pqxx::connection conn{argv[1]};

        // Создаём базу данных, если её нет
        // Создаём транзакцию. Транзакция нужна, чтобы выполнять запросы.
        pqxx::work w(conn);

        // Используя транзакцию создадим таблицу в выбранной базе данных:
        w.exec(
            "CREATE TABLE IF NOT EXISTS books (id SERIAL PRIMARY KEY, title varchar(100) NOT NULL, author varchar(100) NOT NULL, year integer NOT NULL, ISBN varchar(13) UNIQUE);"_zv);

        w.commit();

        conn.prepare(tag_add_book, "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, $4)"_zv); 
        conn.prepare(tag_all_books, "SELECT * FROM books;"_zv);

        // Создаём цикл обработки команд
        while (true) {
            // Read input JSON from std::cin
            std::string inputJson;
            std::getline(std::cin, inputJson);

            // Читаем и разибраем запрос
            Command command;
            // Читаем команду
            try {
                // Parse the input JSON
                json::value parsedJson = json::parse(inputJson);
                command = ReadCommand(inputJson);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing JSON: " << e.what() << std::endl;
                return EXIT_FAILURE;
            }

            if (command.action == "exit") {
                break;
            } else if (command.action == "add_book") {
                std::cout << AddBook(conn, command.payload) << std::endl;
            } else if (command.action == "all_books") {
                std::cout << AllBooks(conn) << std::endl;
            }
        }
        // Нормалдьно вышли из цикла - спокойно выходим
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}