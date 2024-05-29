#include "view.h"

#include <boost/algorithm/string/trim.hpp>
#include <cassert>
#include <iostream>
#include <boost/format.hpp>
#include "../app/use_cases.h"
#include "../menu/menu.h"
#include "../util/tagged_uuid.h"
#include <set>
#include <sstream>
#include <ios>
using boost::format;
using namespace std::literals;
namespace ph = std::placeholders;

namespace ui {
namespace detail {

struct BookTag {};

std::ostream& operator<<(std::ostream& out, const AuthorInfo& author) {
    out << author.name;
    return out;
}

std::ostream& operator<<(std::ostream& out, const BookInfo& book) {
//    out << book.title << ", " << book.publication_year;
    out << book.title << " by " << book.author << ", " << book.publication_year;
    return out;
}

}  // namespace detail

template <typename T>
void PrintVector(std::ostream& out, const std::vector<T>& vector) {
    int i = 1;
    for (auto& value : vector) {
        out << i++ << " " << value << std::endl;
    }
}

View::View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output)
    : menu_{menu}
    , use_cases_{use_cases}
    , input_{input}
    , output_{output} {
    menu_.AddAction(  //
        "AddAuthor"s, "name"s, "Adds author"s, std::bind(&View::AddAuthor, this, ph::_1)
        // ����
        // [this](auto& cmd_input) { return AddAuthor(cmd_input); }
    );
    menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s,
                    std::bind(&View::AddBook, this, ph::_1));
    menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, std::bind(&View::ShowAuthors, this));
    menu_.AddAction("ShowBooks"s, {}, "Show books"s, std::bind(&View::ShowBooks, this));
    menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s,
                    std::bind(&View::ShowAuthorBooks, this));
    menu_.AddAction("DeleteAuthor"s, "name"s, "Deletes author"s, std::bind(&View::DeleteAuthor, this, ph::_1));
    menu_.AddAction("DeleteBook"s, "name"s, "Deletes book"s, std::bind(&View::DeleteBook, this, ph::_1));
    menu_.AddAction("EditAuthor"s, "name"s, "Edit author"s, std::bind(&View::EditAuthor, this, ph::_1));
    menu_.AddAction("ShowBook"s, "name"s, "Show book"s, std::bind(&View::ShowBook, this, ph::_1));
    menu_.AddAction("EditBook"s, "name"s, "Edit book"s, std::bind(&View::EditBook, this, ph::_1));

}

bool View::AddAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        if(name.empty())
        	throw std::exception();
        use_cases_.AddAuthor(std::move(name));
    } catch (const std::exception&) {
        output_ << "Failed to add author"sv << std::endl;
    }
    return true;
}

bool View::DeleteAuthor(std::istream& cmd_input) const {
	try {
	        std::string name;
	        std::getline(cmd_input, name);
	        boost::algorithm::trim(name);
	        std::string author_id;
	        if(name.empty())
	        {
	        	auto id = SelectAuthor();
	            if (not id.has_value())
	            	 throw std::exception();
	            author_id = *id;
	        }
//	        	throw std::exception();
	        else
	        {
	        	const auto& authors = GetAuthors();
	        	auto itFind = std::find_if(authors.begin(), authors.end(), [&name](auto& info)
	        		       	    			{
	        		       	    				return info.name == name;
	        		       	    			});
	        	if(itFind == authors.end())
	        		throw std::exception();

	        	author_id = itFind->id;
	        }
	        //std::cout << "Deleting:" << name << "id:" << itFind->id << std::endl;
	        use_cases_.DeleteAuthor(author_id);
	    } catch (const std::exception& ex) {
	        output_ << "Failed to delete author"sv << std::endl;
	    	//output_ << "Failed to delete author:" << ex.what() << std::endl;
	    }
        return true;
}

bool View::DeleteBook(std::istream& cmd_input) const
{
try{
    std::string name;
    std::getline(cmd_input, name);
    boost::algorithm::trim(name);
    if(name.empty())
    {
    	auto book_id = SelectBook();
    	 if (not book_id.has_value())
    		 return true;
    		// throw std::exception();
    	 name = *book_id;
    }
    else
    {
    	if(!BookExists(name))
    		throw std::exception();
    }
    auto res = use_cases_.GetBookByTitle(name);
    if(res.size() > 0)
    {
//    	std::cout << "View::DeleteBook found: " << res.size() << std::endl;
    	int index = 0;
    	if(res.size() > 1)
    		index = GetBookToShow(res);
    	if(index >= 0)
    		use_cases_.DeleteBook(res[index]);
    }
} catch (const std::exception& ex) {
    output_ << "Book not found"sv << std::endl;
}
	return true;
}

bool View::EditAuthor(std::istream& cmd_input) const
{
try{
    std::string name;
    std::getline(cmd_input, name);
    boost::algorithm::trim(name);

    std::string auth_id;
    if(name.empty())
    {
       	auto id = SelectAuthor();
        if (not id.has_value())
           	 throw std::exception();
        auth_id = *id;
    }
    else
    {
    	auto id = FindAuthorIdByName(name);
    	if (not id.has_value())
    	           	 throw std::exception();
    	auth_id = *id;
    }
    	//throw std::exception();
    output_ << "Enter new name:"sv << std::endl;

    std::getline(input_, name);
    boost::algorithm::trim(name);
    if(name.empty())
   	 throw std::exception();

    use_cases_.EditAuthor(auth_id, name);
}catch (const std::exception& ex) {
    output_ << "Failed to edit author"sv << std::endl;
	//output_ << "Failed to delete author:" << ex.what() << std::endl;
}
	return true;
}

bool View::AddBook(std::istream& cmd_input) const {
    try {
        if (auto params = GetBookParams(cmd_input)) {
        	use_cases_.AddBook(*params);
        }
    } catch (const std::exception& ex) {
//        output_ << "Failed to add book"sv << std::endl;
//    	output_ << ex.what() << std::endl;
    }
    return true;
}

bool View::ShowAuthors() const {
    PrintVector(output_, GetAuthors());
    return true;
}

bool View::ShowBooks() const {
    PrintVector(output_, GetBooks());
    return true;
}

bool View::ShowAuthorBooks() const {
    // TODO: handle error
    try {
        if (auto author_id = SelectAuthor()) {
            PrintVector(output_, GetAuthorBooks(*author_id));
        }
    } catch (const std::exception&) {
        throw std::runtime_error("Failed to Show Books");
    }
    return true;
}

std::optional<detail::AddBookParams> View::GetBookParams(std::istream& cmd_input) const {
    detail::AddBookParams params;

    cmd_input >> params.publication_year;
    std::getline(cmd_input, params.title);
    boost::algorithm::trim(params.title);

    if(params.title.empty() || !params.publication_year)
           	throw std::exception();

    output_ << "Enter author name or empty line to select from list:" << std::endl;
    std::string auth_name;
    std::getline(input_, auth_name);
    boost::algorithm::trim(auth_name);
    std::string id;
    if(auth_name.empty())
    {
    	if(GetAuthors().size())
    	{
    		auto author_id = SelectAuthor();
    		if (not author_id.has_value())
    			throw std::exception();
    		id = *author_id;
    	}
    	else
    		throw std::exception();
    }
    else
    {
    	if(!AuthorFound(auth_name))
    	{
    		//std::string prompt = "No author found. Do you want to add %1% (y/n)?"%auth_name;
    		auto fmt = boost::format("No author found. Do you want to add %1% (y/n)?") % auth_name;
    		output_ << fmt << std::endl;
    		std::string answer;
    		std::getline(input_, answer);
    		boost::algorithm::trim(answer);
    		if((answer != "Y") && (answer != "y"))
    		{
    			output_ << "Failed to add book"sv << std::endl;
    			return std::nullopt;
    		}
    		else
    			use_cases_.AddAuthor(std::move(auth_name));
    	}
    	auto author_id = FindAuthorIdByName(auth_name);
		if (not author_id.has_value())
			throw std::exception();
		id = *author_id;
    }

    output_ << "Enter tags (comma separated):" << std::endl;
    std::string tags;
    std::getline(input_, tags);
    boost::algorithm::trim(tags);
    std::vector<std::string> parsed_tags;
    if(!tags.empty())
    	parsed_tags = NormalizeTags(tags);
 //   std::cout << "Tags: "<< tags << std::endl;
    //if(!tags.empty())
    //	params.tags = tags;
    params.tags = parsed_tags;

    params.author_id = id;
    auto uuid = util::TaggedUUID<ui::detail::BookTag>::New();
    params.id = uuid.ToString();

    ///////////////////////////////////////////////////////////////
 /*   std::cout << "AddBookParams: " << std::endl;
    std::cout << "id:" <<  params.id << std::endl;
    std::cout << "title:" << params.title << std::endl;
	std::cout <<  "author:" <<  params.author_id << std::endl;
	if(!params.tags.empty())
		std::cout <<  "tags:" << params.tags << std::endl;
	else
		std::cout << "tags:" << "null" << std::endl;

    std::cout<< "pub year:" <<   params.publication_year << std::endl; */
    //////////////////////////////////////////////////////////////
    return params;
}

bool View::AuthorFound(const std::string& auth_name) const
{
	auto authors = GetAuthors();
	auto it = std::find_if(authors.begin(), authors.end(), [&auth_name](const auto& auth_info){
		return auth_info.name == auth_name;
	});

	return it != authors.end();
}

std::optional<std::string> View::FindAuthorIdByName(const std::string& auth_name) const
{
	auto authors = GetAuthors();
	auto it = std::find_if(authors.begin(), authors.end(), [&auth_name](const auto& auth_info){
		return auth_info.name == auth_name;
	});

	if(it != authors.end())
		return it->id;
	return std::nullopt;
}

std::optional<std::string> View::SelectAuthor() const {
    output_ << "Select author:" << std::endl;
    auto authors = GetAuthors();
    PrintVector(output_, authors);
    output_ << "Enter author # or empty line to cancel" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int author_idx;
    try {
        author_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::runtime_error("Invalid author num");
    }

    --author_idx;
    if (author_idx < 0 or author_idx >= authors.size()) {
        throw std::runtime_error("Invalid author num");
    }

    return authors[author_idx].id;
}

std::optional<std::string> View::SelectBook() const {
//    output_ << "Select author:" << std::endl;
    auto books = GetBooks();
    PrintVector(output_, books);
    output_ << "Enter the book # or empty line to cancel:" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int book_idx;
    try {
    	book_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::exception();
    }

    --book_idx;
    if (book_idx < 0 or book_idx >= books.size()) {
        throw std::exception();
    }
//    output_ << "SelectBook index:" << book_idx << std::endl;
//    output_ << "SelectBook title:" << books[book_idx].title << std::endl;
    return books[book_idx].title;
}


std::vector<detail::AuthorInfo> View::GetAuthors() const {
    std::vector<detail::AuthorInfo> dst_autors;
    //assert(!"TODO: implement GetAuthors()");
    dst_autors = use_cases_.GetAuthors();
    return dst_autors;
}

std::vector<detail::BookInfo> View::GetBooks() const {
    std::vector<detail::BookInfo> books;
    books = use_cases_.GetBooks();
    return books;
}

std::vector<detail::BookInfo> View::GetAuthorBooks(const std::string& author_id) const {
    std::vector<detail::BookInfo> books;
//    assert(!"TODO: implement GetAuthorBooks()");
    books = use_cases_.GetAuthorBooks(author_id);
    return books;
}

std::string View::NormalizeTag(std::string& word) const
{
    std::string rs;
    std::vector<std::string> res;

    std::istringstream iss(word);
    while (std::getline(iss, word, ' ')) {
        boost::algorithm::trim(word);
        if(word.empty())
            continue;
        res.push_back(word);
    }

    for(auto it = res.begin(); it != res.end(); ++it)
        rs = rs + *it + " ";

    if(rs.size() > 1)
    	rs.erase(rs.size()-1) ;

    return rs;
}



std::vector<std::string> View::NormalizeTags(const std::string& sentence) const
{
	std::vector<std::string> res;
    std::string word;
    std::set<std::string> tags;

    std::istringstream iss(sentence);
    while (std::getline(iss, word, ',')) {
        boost::algorithm::trim(word);
        if(word.empty())
            continue;
        word = NormalizeTag(word);
        tags.insert(word);
    }

    for(auto it = tags.begin(); it != tags.end(); ++it)
    	res.push_back(*it);

     return res;
}

bool View::BookExists(const std::string& title) const {
	auto books = GetBooks();

	auto itFind = std::find_if(books.begin(), books.end(),[&title](auto& book_entry){
		return book_entry.title == title;
	});

	return itFind != books.end();
}

void View::ShowBookInfo(const ui::detail::BookInfo& info) const
{
	output_ << "Title: " <<  info.title << std::endl;
	output_ << "Author: " << info.author << std::endl;
	output_ << "Publication year: " << info.publication_year << std::endl;
	if(info.tags.size())
		output_ << "Tags: " << ConvertTagsToString(info.tags) << std::endl;
}

int View::GetBookToShow(const std::vector<ui::detail::BookInfo>& books) const {
    PrintVector(output_, books);
    output_ << "Enter the book # or empty line to cancel" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
    	//throw std::runtime_error("Invalid book num");
        return -1;
    }

    int book_idx;
    try {
    	book_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::exception();
    }

    --book_idx;
    if (book_idx < 0 or book_idx >= books.size()) {
        throw std::exception();
    }
//    output_ << "Select book:" << book_idx << std::endl;
    return book_idx;
}

bool View::ShowBook(std::istream& cmd_input) const {
try{
    std::string name;
    std::getline(cmd_input, name);
    boost::algorithm::trim(name);

    if(name.empty())
    {
    	auto title = SelectBook();
    	if (not title.has_value())
    		throw std::exception();
    	name = *title;
    //	output_ << "Selected name:" << name << std::endl;
    }
    else
    {
    	if(!BookExists(name))
    		return true;
    }
    auto res = use_cases_.GetBookByTitle(name);
    if(res.size() > 0)
    {
    	int index = 0;
    	if(res.size() > 1)
    		index = GetBookToShow(res);
    	if(index >=0)
    		ShowBookInfo(res[index]);
    }
}catch (const std::exception& ex) {
 	//output_ << "Book not found" << std::endl;
}
	return true;
}

std::string View::GetBookNewName(const std::string& name) const {

	auto fmt = boost::format("Enter new title or empty line to use the current one (%1%):") % name;
    output_ << fmt << std::endl;

    std::string new_name;
/*    std::getline(input_, new_name);
    boost::algorithm::trim(new_name);
*/
    if (!std::getline(input_, new_name) || new_name.empty())
      	return name;

    boost::algorithm::trim(new_name);
    if(new_name.empty())
    	return name;

    return new_name;
}

int View::GetBookNewPubYear(int year) const {

	int publication_year = 0;

	auto fmt = boost::format("Enter publication year or empty line to use the current one (%1%):") % year;
	output_ << fmt << std::endl;

	std::string str;
	if (!std::getline(input_, str) || str.empty()) {
//		output_ << "Year: " << year << std::endl;
		return year;
	}

    try
    {
    	publication_year = std::stoi(str);
	} catch (std::exception const&) {
		throw std::runtime_error("Invalid year");
	}

//	output_ << "Year: " << publication_year << std::endl;
	return publication_year;
}

std::string View::ConvertTagsToString(const std::vector<std::string>& tags) const {

	std::string new_tags;

	for(auto it = tags.begin(); it != tags.end(); ++it)
		new_tags = new_tags + *it + ", ";

	if(new_tags.size() > 2)
		new_tags.erase(new_tags.size()-2) ;

	return new_tags;
}

std::vector<std::string> View::GetNewTags(const std::vector<std::string>& tags) const{

	std::string new_tags = ConvertTagsToString(tags);
	std::vector<std::string> res;

	auto fmt = boost::format("Enter tags (current tags: %1%)") % new_tags;
	output_ << fmt << std::endl;

	std::getline(input_, new_tags);
	boost::algorithm::trim(new_tags);

	if(!new_tags.empty())
		res = NormalizeTags(new_tags);

	return res;
}

std::vector<detail::BookInfo> View::GetBookByTitle(const std::string& title) const
{
	std::vector<detail::BookInfo> res;

	auto books = GetBooks();
	for(auto it = books.begin(); it != books.end(); ++it)
	{
		if(it->title == title)
			res.push_back(*it);
	}

	return res;
}

bool View::EditBook(std::istream& cmd_input) const {

    std::string name;
    std::getline(cmd_input, name);
    boost::algorithm::trim(name);
try{
    if(name.empty())
    {
    	auto title = SelectBook();
    	if (not title.has_value())
    	{
    		output_ << "Book not found" << std::endl;
    		return true;
    	}
    	name = *title;
    }
    else
    {
    	if(!BookExists(name))
    	{
    		//throw std::exception();
    		output_ << "Book not found" << std::endl;
    		return true;
    	}
    }
//	output_ << "Selected name:" << name << std::endl;
    auto books = GetBookByTitle(name);

    if(!books.size())
    	throw std::exception();

    int index = 0;
    if(books.size() > 1)
    	index = GetBookToShow(books);
    if(index >= 0)
    {
    	detail::BookInfo info = books[index];

    	info.title = GetBookNewName(name);
    	info.publication_year = GetBookNewPubYear(info.publication_year);
    	info.tags = GetNewTags(info.tags);

/*
	output_ << "New title:" << info.title << std::endl;
	output_ << "New publication_year:" << info.publication_year << std::endl;
	output_ << "New tags:" << info.tags << std::endl;
*/
    	use_cases_.UpdateBook(books[index], info);
    }
    else
    	output_ << "Book not found" << std::endl;
}catch (const std::exception& ex) {
 //	output_ << "Book not found" << std::endl;
}
	return true;
}

}  // namespace ui
