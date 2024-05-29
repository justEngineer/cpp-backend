#include "postgres.h"

#include <pqxx/pqxx>
#include <pqxx/zview.hxx>
#include <vector>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    // Пока каждое обращение к репозиторию выполняется внутри отдельной транзакции
    // В будущих уроках вы узнаете про паттерн Unit of Work, при помощи которого сможете несколько
    // запросов выполнить в рамках одной транзакции.
    // Вы также может самостоятельно почитать информацию про этот паттерн и применить его здесь.
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO authors (id, name) VALUES ($1, $2)
ON CONFLICT (id) DO UPDATE SET name=$2;
)"_zv,
        author.GetId().ToString(), author.GetName());
    work.commit();
}

domain::Author AuthorRepositoryImpl::LoadById(const domain::AuthorId& id) {
    pqxx::read_transaction r(connection_);
    auto query_text = R"(
SELECT name FROM authors 
WHERE id = )" + id.ToString() + R"( 
LIMIT 1;)";

    auto [name] = r.query1<std::string>(query_text);

    return domain::Author{id, name};
}

std::vector<domain::Author> AuthorRepositoryImpl::GetAllAuthors() {
    pqxx::read_transaction r(connection_);
    auto query_text = R"(SELECT id, name FROM authors ORDER BY name;)";

    std::vector<domain::Author> res;

    for (auto [id, name] : r.query<std::string, std::string>(query_text)) {
        res.emplace_back(domain::Author{domain::AuthorId::FromString(id), name});
    }

    return res;
}

void BookRepositoryImpl::Save(const domain::Book& book) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4)
ON CONFLICT (id) DO UPDATE SET author_id=$2, title=$3, publication_year=$4;
)"_zv,
        book.GetBookId().ToString(), book.GetAuthorId().ToString(), book.GetTitle(), book.GetYear());
    work.commit();
}

domain::Book BookRepositoryImpl::LoadById(const domain::BookId& id) {
    pqxx::read_transaction r(connection_);
    auto query_text = R"(
SELECT author_id, title, publication_year FROM books 
WHERE id = )" + r.quote(id.ToString()) + R"( 
LIMIT 1;)";

    auto [author_id, title, year] = r.query1<std::string, std::string, std::optional<int>>(query_text);

    return domain::Book{id, domain::AuthorId::FromString(author_id), title, year};
}

std::vector<domain::Book> BookRepositoryImpl::GetAllBooks() {
    pqxx::read_transaction r(connection_);
    auto query_text = R"(
SELECT id, author_id, title, publication_year FROM books 
ORDER BY title;)";

    std::vector<domain::Book> res;

    for (auto [id, author_id, title, year] : r.query<std::string, std::string, std::string, std::optional<int>>(query_text)) {
        res.emplace_back(domain::Book{domain::BookId::FromString(id), domain::AuthorId::FromString(author_id), title, year});
    }

    return res;
}

std::vector<domain::Book> BookRepositoryImpl::GetAuthorBooks(const domain::AuthorId& author_id) {
    pqxx::read_transaction r(connection_);
    auto query_text = R"(
SELECT id, author_id, title, publication_year FROM books 
WHERE author_id = )" + r.quote(author_id.ToString()) + R"( 
ORDER BY publication_year, title;)";

    std::vector<domain::Book> res;

    for (auto [id, author_id, title, year] : r.query<std::string, std::string, std::string, std::optional<int>>(query_text)) {
        res.emplace_back(domain::Book{domain::BookId::FromString(id), domain::AuthorId::FromString(author_id), title, year});
    }

    return res;
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
    name varchar(100) UNIQUE NOT NULL
);
)"_zv);

    work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID CONSTRAINT book_id_constraint PRIMARY KEY,
    author_id UUID NOT NULL REFERENCES authors(id),
    title varchar(100) NOT NULL,
    publication_year INTEGER
);
)"_zv);

    // коммитим изменения
    work.commit();
}

}  // namespace postgres