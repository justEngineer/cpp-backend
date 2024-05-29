#include "postgres.h"
#include <string_view>
#include <string>
#include <vector>
#include <pqxx/pqxx>
#include <pqxx/except>
#include "../domain/author.h"
#include <iostream>
#include <set>
#include <boost/algorithm/string/trim.hpp>
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
	params.id, params.title, params.author_id, params.publication_year);

    work.commit();

    AddTag(params);
}

void AuthorRepositoryImpl::AddTag(const ui::detail::AddBookParams& params) {

	for(auto it = params.tags.begin(); it != params.tags.end(); ++it)
	{
		pqxx::work work{connection_};
		work.exec_params(
				R"(INSERT INTO book_tags (book_id, tag) VALUES ($1, $2);)"_zv,
				params.id, *it);
		work.commit();
	}
}

std::vector<ui::detail::BookInfo> AuthorRepositoryImpl::GetBooks()
{
	pqxx::read_transaction rd(connection_);

	auto query_text = R"(SELECT books.id AS book_id, title,  author_id,  authors.name AS name,  publication_year
		FROM books
			INNER JOIN authors ON authors.id = author_id
				ORDER BY title, name, publication_year )"_zv;

	std::vector<ui::detail::BookInfo> res;
	 // Выполняем запрос и итерируемся по строкам ответа
	 for (auto [id, title, auth_id, auth_name, publication_year] : rd.query<std::string, std::string, std::string, std::string, int>(query_text))
	 {
	   ui::detail::BookInfo book{title, auth_name, publication_year};
	   res.push_back(book);
	 }

	return res;
}

std::vector<ui::detail::BookInfo> AuthorRepositoryImpl::GetAuthorBooks(const std::string& author_id)
{
	pqxx::read_transaction rd(connection_);
    auto query_text = "SELECT id, title, author_id, publication_year FROM books WHERE author_id = "+rd.quote(author_id)+" ORDER BY publication_year, title";

	std::vector<ui::detail::BookInfo> res;
	// Выполняем запрос и итерируемся по строкам ответа
	 for (auto [id, title, auth_id, publication_year] : rd.query<std::string, std::string, std::string, int>(query_text))
	 {
	   ui::detail::BookInfo book{title, "", publication_year};
	   res.push_back(book);
	 }

	return res;
}

std::vector<std::string> AuthorRepositoryImpl::GetAuthorBooksIds(const std::string& author_id)
{
	pqxx::read_transaction rd(connection_);
    //auto query_text = "SELECT id, title, author_id, publication_year FROM books WHERE author_id = "+rd.quote(author_id)+" ORDER BY publication_year, title";
	auto query_text = "SELECT id, author_id FROM books WHERE author_id = "+rd.quote(author_id)+" ORDER BY publication_year, title";

	std::vector<std::string> res;
	// Выполняем запрос и итерируемся по строкам ответа
	 for (auto [id, auth_id] : rd.query<std::string, std::string>(query_text))
	 {
//		 std::cout << "AuthorRepositoryImpl::GetAuthorBooksIds: " << id << std::endl;
		 res.push_back(id);
	 }
	return res;
}

void AuthorRepositoryImpl::DeleteAuthorBooks(const std::vector<std::string>& ids/*, pqxx::work& work*/)
{
	 pqxx::work work{connection_};
	 for(auto it = ids.begin(); it != ids.end(); ++it)
	 {
		 work.exec_params(R"(DELETE FROM books WHERE id = $1;)"_zv, *it);
	 }
     work.commit();
}

void AuthorRepositoryImpl::DeleteAuthorBooksTags(const std::string& book_id/*, pqxx::work& work*/)
{
	 pqxx::work work{connection_};
//	std::cout << "AuthorRepositoryImpl::DeleteAuthorBooksTags: " << book_id << std::endl;
     work.exec_params(R"(DELETE FROM book_tags WHERE book_id = $1;)"_zv, book_id);

     work.commit();
}

std::vector<ui::detail::BookInfo> AuthorRepositoryImpl::GetBookByTitle(const std::string& book_title)
{
	pqxx::read_transaction rd(connection_);
 //   auto query_text = "SELECT id, title, author_id, publication_year FROM books WHERE author_id = "+rd.quote(book_title)+" ORDER BY publication_year, title";

   std::vector<std::string> tag_query = {
    "SELECT books.id AS book_id, title, name, publication_year, grouped_tags.tags ",
	"FROM books INNER JOIN authors ON authors.id = author_id ",
	"INNER JOIN ( SELECT book_id, STRING_AGG(tag, ', ') AS tags FROM book_tags GROUP BY book_tags.book_id) AS grouped_tags ",
    "ON grouped_tags.book_id = books.id "};

    std::string where_text = "WHERE title = "+rd.quote(book_title)+";";

    std::string whole_tag_query;
    for(auto it = tag_query.begin(); it != tag_query.end(); ++it)
    {
    	whole_tag_query += *it;
    }
    whole_tag_query += where_text;

//	std::cout << "Query:" << whole_tag_query << std::endl;

    std::vector<ui::detail::BookInfo> res;
    for (auto [id, title, name, publication_year, tags] : rd.query<std::string, std::string, std::string, int, std::string>(whole_tag_query))
    {
       ui::detail::BookInfo book{title, name, publication_year, GetTags(tags)};
       res.push_back(book);
  //     return res;
    }

    if(!res.empty())
    	return res;

    std::string new_tag_query =
    		"SELECT books.id AS book_id, title, name, publication_year FROM books INNER JOIN authors ON authors.id = author_id WHERE title ="+rd.quote(book_title)+";";

    for (auto [id, title, name, publication_year] : rd.query<std::string, std::string, std::string, int>(new_tag_query))
    {
       ui::detail::BookInfo book{title, name, publication_year, {}};
       res.push_back(book);
    }

	return res;
}

void AuthorRepositoryImpl::DeleteAuthor(const std::string& id) {

//	std::cout << "AuthorRepositoryImpl::DeleteAuthor: " << id << std::endl;
	auto bookIds = GetAuthorBooksIds(id);

	for(auto it = bookIds.begin(); it != bookIds.end(); ++it)
		DeleteAuthorBooksTags(*it/*, work*/);

	DeleteAuthorBooks(bookIds/*, work*/);

	pqxx::work work{connection_};
    work.exec_params(
        R"(DELETE FROM authors WHERE id = $1;)"_zv, id);

    work.commit();
}

void AuthorRepositoryImpl::EditAuthor(const std::string& auth_id,const std::string& name)
{
	pqxx::work work{connection_};
	work.exec_params(R"(UPDATE authors SET name = $1 WHERE id = $2;)"_zv, name,  auth_id);

	work.commit();
}

void AuthorRepositoryImpl::DeleteBook(const ui::detail::BookInfo& info)
{
	std::vector<std::string> bookIds;
	{
		pqxx::read_transaction rd(connection_);
		//auto query_text = "SELECT id, title, author_id, publication_year FROM books WHERE author_id = "+rd.quote(author_id)+" ORDER BY publication_year, title";
		//auto query_text = "SELECT id FROM books WHERE title = "+rd.quote(info.title)+" AND author_id = " + rd.quote(info.author) + " AND publication_year = " + rd.quote(info.publication_year) +";";
		auto query_text = "SELECT id FROM books WHERE title = "+rd.quote(info.title) + " AND publication_year = " + rd.quote(info.publication_year) +";";
		// Выполняем запрос и итерируемся по строкам ответа
		for (auto [id] : rd.query<std::string>(query_text))
		{
			//		 std::cout << "AuthorRepositoryImpl::DeleteBook: " << id << std::endl;
			bookIds.push_back(id);
		}
	}
	 for(auto it = bookIds.begin(); it != bookIds.end(); ++it)
	 		DeleteAuthorBooksTags(*it);

	 for(auto it = bookIds.begin(); it != bookIds.end(); ++it)
	 {
		 pqxx::work work{connection_};
	     work.exec_params(R"(DELETE FROM books WHERE id = $1;)"_zv, *it);
	     work.commit();
	 }
}

void AuthorRepositoryImpl::EditTag(const std::string& book_id,const std::vector<std::string>& new_tag)
{
	{
		pqxx::work work{connection_};
		//work.exec_params(R"(UPDATE book_tags SET tag = $1 WHERE book_id = $2;)"_zv, new_tag,  book_id);
		work.exec_params(R"(DELETE FROM book_tags WHERE book_id = $1;)"_zv, book_id);
		work.commit();
	}

	for(auto it = new_tag.begin(); it != new_tag.end(); ++it)
	{
		pqxx::work work{connection_};
	    work.exec_params(R"(INSERT INTO book_tags (book_id, tag) VALUES ($1, $2);)"_zv, book_id, *it);
	    work.commit();
	}
}

void AuthorRepositoryImpl::UpdateBook(const ui::detail::BookInfo& old_info, const ui::detail::BookInfo& new_info)
{
	std::string book_id;
	{
		pqxx::read_transaction rd(connection_);
		auto query_text = "SELECT id FROM books WHERE title = "+rd.quote(old_info.title)+" AND publication_year = " + std::to_string(old_info.publication_year) +";";
//		std::cout << "AuthorRepositoryImpl::query_text: " << query_text << std::endl;

		// Выполняем запрос и итерируемся по строкам ответа
		for (auto [id] : rd.query<std::string>(query_text))
		{
//			std::cout << "AuthorRepositoryImpl::UpdateBook: " << id << std::endl;
			book_id = id;
		}
	}
	EditTag(book_id, new_info.tags);

	pqxx::work work{connection_};
	work.exec_params(R"(UPDATE books SET title = $1, publication_year = $2 WHERE id = $3;)"_zv, new_info.title,  new_info.publication_year, book_id);

	work.commit();

}

std::vector<std::string> AuthorRepositoryImpl::GetTags(const std::string& sentence)
{
	std::vector<std::string> res;
    std::string word;
    std::set<std::string> tags;

    std::istringstream iss(sentence);
    while (std::getline(iss, word, ',')) {
        boost::algorithm::trim(word);
        if(word.empty())
            continue;

        tags.insert(word);
    }

    for(auto it = tags.begin(); it != tags.end(); ++it)
    	res.push_back(*it);

     return res;
}

void AuthorRepositoryImpl::DeleteTables()
{
	pqxx::work work{connection_};
	work.exec_params(R"(DELETE from book_tags;)"_zv);
	work.exec_params(R"(DELETE from books;)"_zv);
	work.exec_params(R"(DELETE from authors; )"_zv);
	work.commit();
}

//https://github.com/AndrewSalsaDancer2023/CppBackend/actions/runs/4896668783/jobs/8743765896
//https://github.com/AndrewSalsaDancer2023/CppBackend/actions/runs/4902168655/jobs/8753852539
Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
                        id UUID CONSTRAINT firstindex PRIMARY KEY,
                        name varchar(100) NOT NULL UNIQUE
                    );
)"_zv);
    // ... создать другие таблицы

    work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
                       id UUID PRIMARY KEY,
                       title VARCHAR(100) NOT NULL,
                       publication_year INT,
                       author_id UUID,
                       CONSTRAINT fk_authors
                           FOREIGN KEY(author_id)
                           REFERENCES authors(id)
                    );
)"_zv);

    work.exec(R"(
CREATE TABLE IF NOT EXISTS book_tags (
                      book_id UUID,
                      tag varchar(30) NOT NULL,
                      CONSTRAINT fk_books
                          FOREIGN KEY(book_id)
                          REFERENCES books(id)
                    );
)"_zv);


    // коммитим изменения
    work.commit();
}

}  // namespace postgres
