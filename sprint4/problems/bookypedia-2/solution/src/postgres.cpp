#include "postgres.h"
#include <string_view>
#include <string>
//#include <pqxx/zview.hxx>
#include <pqxx/pqxx>
#include <pqxx/except>
#include "../domain/author.h"

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

std::vector<ui::detail::AuthorInfo> AuthorRepositoryImpl::Load() {

	pqxx::read_transaction rd(connection_);
	auto query_text = "SELECT id, name FROM authors ORDER BY name;"_zv;

	std::vector<ui::detail::AuthorInfo> res;
	 // Выполняем запрос и итерируемся по строкам ответа
	   for (auto [id, name] : rd.query<std::string, std::string>(query_text))
	   {
		   ui::detail::AuthorInfo author{id, name};
		   res.push_back(author);
	   }

	return res;
}

void AuthorRepositoryImpl::AddBook(const ui::detail::AddBookParams& params) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO books (id, title, author_id, publication_year) VALUES ($1, $2, $3, $4);
)"_zv,
	params.id, params.author_id, params.title,  params.publication_year);
    work.commit();
}

std::vector<ui::detail::BookInfo> AuthorRepositoryImpl::GetBooks()
{
	pqxx::read_transaction rd(connection_);
	auto query_text = "SELECT title, publication_year FROM books ORDER BY title;"_zv;

	std::vector<ui::detail::BookInfo> res;
	 // Выполняем запрос и итерируемся по строкам ответа
	 for (auto [title, publication_year] : rd.query<std::string, int>(query_text))
	 {
	   ui::detail::BookInfo book{title, publication_year};
	   res.push_back(book);
	 }

	return res;
}

std::vector<ui::detail::BookInfo> AuthorRepositoryImpl::GetAuthorBooks(const std::string& author_id)
{
	pqxx::read_transaction rd(connection_);
    auto query_text = "SELECT title, publication_year FROM books WHERE author_id = "+rd.quote(author_id)+" ORDER BY publication_year, title;";
	std::vector<ui::detail::BookInfo> res;
	// Выполняем запрос и итерируемся по строкам ответа
	 for (auto [title, publication_year] : rd.query<std::string, int>(query_text))
	 {
	   ui::detail::BookInfo book{title, publication_year};
	   res.push_back(book);
	 }

	return res;
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
    name varchar(100) NOT NULL UNIQUE
);
)"_zv);

    work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID PRIMARY KEY,
    title varchar(100) NOT NULL,
	publication_year INT,
	author_id UUID,
    CONSTRAINT fk_authors
		FOREIGN KEY(author_id)
		REFERENCES authors(id)
);
)"_zv);

    // коммитим изменения
    work.commit();
}

}  // namespace postgres
