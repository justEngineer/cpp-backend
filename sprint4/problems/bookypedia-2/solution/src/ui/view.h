#pragma once
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace menu {
class Menu;
}

namespace app {
class UseCases;
}

namespace ui {
namespace detail {

struct AddBookParams {
	std::string id;
    std::string title;
    std::string author_id;
    std::vector<std::string> tags;
    int publication_year = 0;
};

struct AuthorInfo {
    std::string id;
    std::string name;
};

struct BookInfo {
    std::string title;
    std::string author;
    int publication_year = 0;
    std::vector<std::string> tags;
};

}  // namespace detail

class View {
public:
    View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output);

private:
    bool AddAuthor(std::istream& cmd_input) const;
    bool AddBook(std::istream& cmd_input) const;
    bool ShowAuthors() const;
    bool ShowBooks() const;
    bool ShowAuthorBooks() const;
    bool DeleteAuthor(std::istream& cmd_input) const;
    bool DeleteBook(std::istream& cmd_input) const;
    bool EditAuthor(std::istream& cmd_input) const;
    bool ShowBook(std::istream& cmd_input) const;
    bool BookExists(const std::string& title ) const;
    std::optional<detail::AddBookParams> GetBookParams(std::istream& cmd_input) const;
    bool EditBook(std::istream& cmd_input) const;
    bool AuthorFound(const std::string& auth_name) const;
    std::optional<std::string> FindAuthorIdByName(const std::string& auth_name) const;
    std::optional<std::string> SelectAuthor() const;
    std::optional<std::string> SelectBook() const;
    std::vector<detail::AuthorInfo> GetAuthors() const;
    std::vector<detail::BookInfo> GetBooks() const;
    std::vector<detail::BookInfo> GetAuthorBooks(const std::string& author_id) const;
    std::string NormalizeTag(std::string& word) const;
    std::vector<std::string> NormalizeTags(const std::string& sentence) const;
    void ShowBookInfo(const ui::detail::BookInfo& info) const;
    int GetBookToShow(const std::vector<ui::detail::BookInfo>& books) const;
    std::string GetBookNewName(const std::string& name) const;
    int GetBookNewPubYear(int year) const;
    std::vector<std::string> GetNewTags(const std::vector<std::string>& tags) const;
    std::string ConvertTagsToString(const std::vector<std::string>& tags) const;
    std::vector<detail::BookInfo> GetBookByTitle(const std::string& title) const;
    menu::Menu& menu_;
    app::UseCases& use_cases_;
    std::istream& input_;
    std::ostream& output_;
};

}  // namespace ui
