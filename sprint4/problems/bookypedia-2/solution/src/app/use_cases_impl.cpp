#include "use_cases_impl.h"

#include "../domain/author.h"

namespace app {
using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

std::vector<ui::detail::AuthorInfo> UseCasesImpl::GetAuthors()
{
	std::vector<ui::detail::AuthorInfo> res  = authors_.Load();
	return res;
}

void UseCasesImpl::AddBook(const ui::detail::AddBookParams& params)
{
	authors_.AddBook(params);
}

std::vector<ui::detail::BookInfo> UseCasesImpl::GetBooks()
{
	std::vector<ui::detail::BookInfo> res = authors_.GetBooks();
	return res;
}

std::vector<ui::detail::BookInfo> UseCasesImpl::GetAuthorBooks(const std::string& author_id)
{
	std::vector<ui::detail::BookInfo> res = authors_.GetAuthorBooks(author_id);
	return res;
}

void UseCasesImpl::DeleteAuthor(const std::string& id) {
	authors_.DeleteAuthor(id);
}

void UseCasesImpl::EditAuthor(const std::string& auth_id,const std::string& name) {
	authors_.EditAuthor(auth_id, name);
}

std::vector<ui::detail::BookInfo> UseCasesImpl::GetBookByTitle(const std::string& book_title) {
	return authors_.GetBookByTitle(book_title);
}

void UseCasesImpl::DeleteBook(const ui::detail::BookInfo& info) {
	authors_.DeleteBook(info);
}

void UseCasesImpl::UpdateBook(const ui::detail::BookInfo& old_info, const ui::detail::BookInfo& new_info) {
	authors_.UpdateBook(old_info, new_info);
}

}  // namespace app
