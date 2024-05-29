#pragma once
#include "../domain/author_fwd.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(domain::AuthorRepository& authors)
        : authors_{authors} {
    }

    void AddAuthor(const std::string& name) override;
    std::vector<ui::detail::AuthorInfo> GetAuthors() override;
    void AddBook(const ui::detail::AddBookParams& params) override;
    std::vector<ui::detail::BookInfo> GetBooks() override;
    std::vector<ui::detail::BookInfo> GetAuthorBooks(const std::string& author_id) override;
    void DeleteAuthor(const std::string& id) override;
    void EditAuthor(const std::string& auth_id,const std::string& name) override;
    std::vector<ui::detail::BookInfo> GetBookByTitle(const std::string& book_title) override;
    void DeleteBook(const ui::detail::BookInfo& info) override;
    void UpdateBook(const ui::detail::BookInfo& old_info, const ui::detail::BookInfo& new_info) override;
private:
    domain::AuthorRepository& authors_;
};

}  // namespace app
