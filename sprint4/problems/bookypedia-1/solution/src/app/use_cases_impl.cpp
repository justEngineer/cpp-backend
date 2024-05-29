#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

std::vector<domain::Author> UseCasesImpl::GetAllAuthors() {
    return authors_.GetAllAuthors();
}

void UseCasesImpl::AddBook(const std::string& author_id, const std::string& title, int publication_year) {
    books_.Save({BookId::New(), AuthorId::FromString(author_id), title, publication_year});
}

std::vector<domain::Book> UseCasesImpl::GetAllBooks() {
    return books_.GetAllBooks();
}

std::vector<domain::Book> UseCasesImpl::GetAuthorBooks(const std::string& author_id) {
    return books_.GetAuthorBooks(AuthorId::FromString(author_id));
}

}  // namespace app
