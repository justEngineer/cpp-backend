#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>
#include "../domain/author.h"

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(const domain::Author& author) override;
    std::vector<ui::detail::AuthorInfo> Load() override;
    void AddBook(const ui::detail::AddBookParams& params) override;
    virtual std::vector<ui::detail::BookInfo> GetBooks() override;
    std::vector<ui::detail::BookInfo> GetAuthorBooks(const std::string& author_id) override;

    std::vector<std::string> GetAuthorBooksIds(const std::string& author_id);
    void DeleteAuthorBooks(const std::vector<std::string>& ids/*, pqxx::work& work*/);
    void DeleteAuthorBooksTags(const std::string& book_id/*, pqxx::work& work*/);

    std::vector<ui::detail::BookInfo> GetBookByTitle(const std::string& book_title) override;
    void AddTag(const ui::detail::AddBookParams& params) override;
    void DeleteAuthor(const std::string& id) override;
    void EditAuthor(const std::string& auth_id,const std::string& name) override;
    void EditTag(const std::string& book_id,const std::vector<std::string>& new_tag);
    void DeleteBook(const ui::detail::BookInfo& info) override;
    void UpdateBook(const ui::detail::BookInfo& old_info, const ui::detail::BookInfo& new_info) override;
    std::vector<std::string> GetTags(const std::string& sentence);
    void DeleteTables();
private:
    pqxx::connection& connection_;
};

class Database {
public:
    explicit Database(pqxx::connection connection);

    AuthorRepositoryImpl& GetAuthors() & {
        return authors_;
    }

private:
    pqxx::connection connection_;
    AuthorRepositoryImpl authors_{connection_};
};

}  // namespace postgres
