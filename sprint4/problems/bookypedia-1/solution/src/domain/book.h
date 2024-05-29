#pragma once

#include "author.h"

#include <string>
#include <vector>
#include <optional>

#include "../util/tagged_uuid.h"

namespace domain {

namespace detail {
struct BookTag {};
}  // namespace detail

using BookId = util::TaggedUUID<detail::BookTag>;

class Book {
public:
    Book(BookId b_id, AuthorId a_id, std::string title, std::optional<int> year)
        : id_(std::move(b_id))
        , author_id_(std::move(a_id))
        , title_(std::move(title))
        , year_(year) {
    }

    const BookId& GetBookId() const noexcept {
        return id_;
    }

    const AuthorId& GetAuthorId() const noexcept {
        return author_id_;
    }

    const std::string& GetTitle() const noexcept {
        return title_;
    }

    const std::optional<int>& GetYear() const noexcept {
        return year_;
    }

private:
    BookId id_;
    AuthorId author_id_;
    std::string title_;
    std::optional<int> year_;
};

class BookRepository {
public:
    virtual void Save(const Book& book) = 0;
    virtual Book LoadById(const BookId& id) = 0;
    virtual std::vector<Book> GetAllBooks() = 0;
    virtual std::vector<Book> GetAuthorBooks(const AuthorId& author_id) = 0;

protected:
    ~BookRepository() = default;
};

}  // namespace domain
