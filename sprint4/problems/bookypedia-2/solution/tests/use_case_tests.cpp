#include <catch2/catch_test_macros.hpp>

#include "../src/app/use_cases_impl.h"
#include "../src/domain/author.h"

namespace {

struct MockAuthorRepository : domain::AuthorRepository {
    std::vector<domain::Author> saved_authors;

    void Save(const domain::Author& author) override {
        saved_authors.emplace_back(author);
    }
    std::vector<ui::detail::AuthorInfo> Load() override {
    	return {};
    }

    void AddBook(const ui::detail::AddBookParams& params) {

    }

    std::vector<ui::detail::BookInfo> GetBooks() override {
    	return {};
    }

    std::vector<ui::detail::BookInfo> GetAuthorBooks(const std::string& author_id) override {
    	return {};
    }

    std::vector<ui::detail::BookInfo> GetBookByTitle(const std::string& book_title) override {
    	return {};
    }

    void AddTag(const ui::detail::AddBookParams& params) override {}

    void DeleteAuthor(const std::string& id) override {}
    void EditAuthor(const std::string& auth_id,const std::string& name) override {}
    void DeleteBook(const ui::detail::BookInfo& info) override {}
    void UpdateBook(const ui::detail::BookInfo& old_info, const ui::detail::BookInfo& new_info) override {}
};

struct Fixture {
    MockAuthorRepository authors;
};

}  // namespace

SCENARIO_METHOD(Fixture, "Book Adding") {
    GIVEN("Use cases") {
        app::UseCasesImpl use_cases{authors};

        WHEN("Adding an author") {
            const auto author_name = "Joanne Rowling";
            use_cases.AddAuthor(author_name);

            THEN("author with the specified name is saved to repository") {
                REQUIRE(authors.saved_authors.size() == 1);
                CHECK(authors.saved_authors.at(0).GetName() == author_name);
                CHECK(authors.saved_authors.at(0).GetId() != domain::AuthorId{});
            }
        }
    }
}
