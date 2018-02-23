#include <iostream>
#include <map>

#include <args.hxx>

#include <restinio/all.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

const char *
content_type_by_file_extention( const restinio::string_view_t & ext )
{
	// Incomplete list of mime types from here:
	// https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types/Complete_list_of_MIME_types
	if(ext == "aac" ) return "audio/aac";
	if(ext == "abw" ) return "application/x-abiword";
	if(ext == "arc" ) return "application/octet-stream";
	if(ext == "avi" ) return "video/x-msvideo";
	if(ext == "azw" ) return "application/vnd.amazon.ebook";
	if(ext == "bin" ) return "application/octet-stream";
	if(ext == "bz" ) return "application/x-bzip";
	if(ext == "bz2" ) return "application/x-bzip2";
	if(ext == "csh" ) return "application/x-csh";
	if(ext == "css" ) return "text/css";
	if(ext == "csv" ) return "text/csv";
	if(ext == "doc" ) return "application/msword";
	if(ext == "docx" ) return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
	if(ext == "eot" ) return "application/vnd.ms-fontobject";
	if(ext == "epub" ) return "application/epub+zip";
	if(ext == "gif" ) return "image/gif";
	if(ext == "htm" || ext == "html" ) return "text/html";
	if(ext == "ico" ) return "image/x-icon";
	if(ext == "ics" ) return "text/calendar";
	if(ext == "jar" ) return "application/java-archive";
	if(ext == "jpeg" || ext == "jpg" ) return "image/jpeg";
	if(ext == "js" ) return "application/javascript";
	if(ext == "json" ) return "application/json";
	if(ext == "mid" || ext == "midi" ) return "audio/midi";
	if(ext == "mpeg" ) return "video/mpeg";
	if(ext == "mpkg" ) return "application/vnd.apple.installer+xml";
	if(ext == "odp" ) return "application/vnd.oasis.opendocument.presentation";
	if(ext == "ods" ) return "application/vnd.oasis.opendocument.spreadsheet";
	if(ext == "odt" ) return "application/vnd.oasis.opendocument.text";
	if(ext == "oga" ) return "audio/ogg";
	if(ext == "ogv" ) return "video/ogg";
	if(ext == "ogx" ) return "application/ogg";
	if(ext == "otf" ) return "font/otf";
	if(ext == "png" ) return "image/png";
	if(ext == "pdf" ) return "application/pdf";
	if(ext == "ppt" ) return "application/vnd.ms-powerpoint";
	if(ext == "pptx" ) return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
	if(ext == "rar" ) return "archive	application/x-rar-compressed";
	if(ext == "rtf" ) return "application/rtf";
	if(ext == "sh" ) return "application/x-sh";
	if(ext == "svg" ) return "image/svg+xml";
	if(ext == "swf" ) return "application/x-shockwave-flash";
	if(ext == "tar" ) return "application/x-tar";
	if(ext == "tif" || ext == "tiff" ) return "image/tiff";
	if(ext == "ts" ) return "application/typescript";
	if(ext == "ttf" ) return "font/ttf";
	if(ext == "vsd" ) return "application/vnd.visio";
	if(ext == "wav" ) return "audio/x-wav";
	if(ext == "weba" ) return "audio/webm";
	if(ext == "webm" ) return "video/webm";
	if(ext == "webp" ) return "image/webp";
	if(ext == "woff" ) return "font/woff";
	if(ext == "woff2" ) return "font/woff2";
	if(ext == "xhtml" ) return "application/xhtml+xml";
	if(ext == "xls" ) return "application/vnd.ms-excel";
	if(ext == "xlsx" ) return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
	if(ext == "xml" ) return "application/xml";
	if(ext == "xul" ) return "application/vnd.mozilla.xul+xml";
	if(ext == "zip" ) return "archive	application/zip";
	if(ext == "3gp" ) return "video/3gpp";
	if(ext == "3g2" ) return "video/3gpp2";
	if(ext == "7z" ) return "application/x-7z-compressed";

	return "application/text";
}

namespace rr = restinio::router;
using router_t = rr::express_router_t<>;

auto server_handler( const std::string & root_dir )
{
	auto router = std::make_unique< router_t >();

	std::string server_root_dir;

	if( root_dir.empty() )
	{
		server_root_dir = "./";
	}
	else if( root_dir.back() != '/' && root_dir.back() != '\\' )
	{
		server_root_dir = root_dir + '/';
	}
	else
	{
		server_root_dir = root_dir;
	}

	// GET request to homepage.
	router->http_get(
		R"(/:path(.*)\.:ext(.*))",
		restinio::path2regex::options_t{}.strict( true ),
		[ server_root_dir ]( auto req, auto params ){

			auto path = req->header().path();

			if( std::string::npos == path.find( ".." ) )
			{
				// A nice path.

				const auto file_path =
					server_root_dir +
						std::string{ path.data(), path.size() };

				auto sf =
					restinio::sendfile(
						file_path,
						restinio::open_file_errh_t::ignore_err );

				if( sf.is_valid() )
				{
					return
						req->create_response()
							.append_header_date_field()
							.append_header(
								restinio::http_field::server,
								"RESTinio" )
							.append_header(
								restinio::http_field::content_type,
								content_type_by_file_extention( params[ "ext" ] ) )
							.set_body( std::move( sf ) )
							.done();
				}
				else
				{
					return
						req->create_response( 404, "Not Found" )
							.append_header_date_field()
							.connection_close()
							.done();
				}
			}
			else
			{
				// Bad path.
				return
					req->create_response( 403, "Forbidden" )
						.append_header_date_field()
						.connection_close()
						.done();
			}

		} );

	router->non_matched_request_handler(
		[]( auto req ){
			if( restinio::http_method_get() == req->header().method() )
				return
					req->create_response( 404, "Not Found" )
						.append_header_date_field()
						.connection_close()
						.done();

			return req->create_response( 501, "Not Implemented" )
					.append_header_date_field()
					.connection_close()
					.done();
		} );

	return router;
}

//
// app_args_t
//

struct app_args_t
{
	bool m_help{ false };
	std::uint16_t m_port{ 8080 };
	std::size_t m_pool_size{ 1 };
	std::string m_root_dir{ "." };

	app_args_t( int argc, const char * argv[] )
	{
		args::ArgumentParser parser( "RESTinio sendfile sample", "" );
		args::HelpFlag help( parser, "Help", "Usage example", { 'h', "help" } );

		args::ValueFlag< std::uint16_t > arg_port(
				parser, "port", "tcp port to run server on (default: 8080)",
				{ 'p', "port" } );

		args::ValueFlag< std::size_t > arg_pool_size(
				parser, "size",
				"The size of a thread pool to run the server",
				{ 'n', "thread-pool-size" } );

		args::ValueFlag< std::string > arg_root_dir(
				parser, "root", "server root dir (default: '.')",
				{ 'r', "root-dir" } );

		try
		{
			parser.ParseCLI( argc, argv );
		}
		catch( const args::Help & )
		{
			m_help = true;
			std::cout << parser;
		}

		if( arg_port )
			m_port = args::get( arg_port );

		if( arg_pool_size )
			m_pool_size = args::get( arg_pool_size );

		if( arg_root_dir )
			m_root_dir = args::get( arg_root_dir );
	}
};

int main( int argc, char const *argv[] )
{
	using namespace std::chrono;

	try
	{
		const app_args_t args{ argc, argv };

		if( !args.m_help )
		{
			using traits_t =
				restinio::traits_t<
					restinio::asio_timer_manager_t,
					restinio::null_logger_t,
					router_t >;

			restinio::run(
				restinio::on_thread_pool< traits_t >( args.m_pool_size )
					.address( "localhost" )
					.port( args.m_port )
					.request_handler( server_handler( args.m_root_dir ) )
					.read_next_http_message_timelimit( 10s )
					.write_http_response_timelimit( 1s )
					.handle_request_timeout( 1s ) );
		}
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
