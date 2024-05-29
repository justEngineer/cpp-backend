#pragma once

#include <string>
#include <vector>
#include "../ui/view.h"
namespace app {

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual std::vector<ui::detail::AuthorInfo> GetAuthors() = 0;
    virtual void AddBook(const ui::detail::AddBookParams& params) = 0;
    virtual std::vector<ui::detail::BookInfo> GetBooks() = 0;
    virtual std::vector<ui::detail::BookInfo> GetAuthorBooks(const std::string& author_id) = 0;
    virtual void DeleteAuthor(const std::string& id) = 0;
    virtual void EditAuthor(const std::string& auth_id,const std::string& name) = 0;
    virtual std::vector<ui::detail::BookInfo> GetBookByTitle(const std::string& book_title) = 0;
    virtual void DeleteBook(const ui::detail::BookInfo& info) = 0;
    virtual void UpdateBook(const ui::detail::BookInfo& old_info, const ui::detail::BookInfo& new_info) = 0;
protected:
    ~UseCases() = default;
};

}  // namespace app
