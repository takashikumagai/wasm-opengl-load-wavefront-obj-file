#ifndef __wavefront_obj_reader_HPP__
#define __wavefront_obj_reader_HPP__


#include <vector>
#include <string>


typedef std::array<float,2> vec2;
typedef std::array<float,3> vec3;


/**
 * separate a given string by separators
 * \param pSrcStr [in] src string
 * \param rDestStr [out] dest buffer
 *
 * example:
 *  SeparateStrings( buf, "pos	1.0 2.0 -3.0", " \t" );
 *  then, you will get buf = [ "pos", "1.0", "2.0", "3.0" ]
 */
inline void SeparateStrings( std::vector<std::string>& rDestStr, const char *pSrcStr, const char *pSeparators )
{
	if( !pSrcStr || !pSeparators )
		return;

	size_t i, str_len = strlen(pSrcStr);
	size_t j, next_pos = 0, num_separaters = strlen(pSeparators);
	char str[256];

	for( i=0; i<str_len; i++ )
	{
		for( j=0; j<num_separaters; j++ )
		{
			if( pSrcStr[i] == pSeparators[j] )
				break;
		}

		if( j < num_separaters )
		{
			if( next_pos < i )
			{
				strncpy( str, pSrcStr + next_pos, i - next_pos );
				str[i - next_pos] = '\0';
				rDestStr.push_back( std::string( str ) );
			}
			next_pos = i + 1;
		}
	}

	// add the last separated string
	if( next_pos < i )
		rDestStr.push_back( std::string( pSrcStr + next_pos ) );
}


//float stof( const std::string& src )
//{
//	return (float)atof(src.c_str());
//}


bool read_wavefront_obj_from_file(
	const std::string& file_pathname,
	std::vector<vec3>& positions,
	std::vector<vec2>& texture_uvs,
	std::vector<vec3>& normals,
	std::vector< std::vector<int> >& polygon_indices
	)
{
	using std::stof;
	using std::stoi;

	FILE *fp = fopen( file_pathname.c_str(), "r" );
	if( !fp )
		return false;

	std::vector<std::string> elements;
	static const int line_buffer_size = 1024;
	char line_buffer[line_buffer_size];
	memset( line_buffer, sizeof(line_buffer), 0 );
	while( fgets( line_buffer, line_buffer_size-1, fp ) )
	{
		elements.resize( 0 );
		SeparateStrings( elements, line_buffer, " \t" );
		if( elements.empty() )
			continue;

		const std::string& tag = elements.front();
		if( tag == "v" && 4 <= elements.size() )
		{
			positions.push_back( { stof(elements[1]),  stof(elements[2]),  stof(elements[3]) } );
		}
		else if( tag == "vt" && 3 <= elements.size() )
		{
			texture_uvs.push_back( { stof(elements[1]),  stof(elements[2]) } );
		}
		else if( tag == "vn" && 4 <= elements.size() )
		{
			normals.push_back( { stof(elements[1]),  stof(elements[2]),  stof(elements[3]) } );
		}
		else if( tag == "f" && 3 <= elements.size() )
		{
			polygon_indices.push_back( std::vector<int>() );
			const int num_indices = (int)elements.size() - 1;
			for( int j=0; j<num_indices; j++ )
			{
				polygon_indices.back().push_back( stoi(elements[j+1]) - 1 );
			}
		}
	}

	return false;
}


#endif /* __wavefront_obj_reader_HPP__ */
