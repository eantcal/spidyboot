/*
 * Copyright (C) 2012 acaldmail@gmail.com
 *
 * Author: Antonino Calderone <acaldmail@gmail.com>
 *    
 * Version 1.0    Initial release
 *
 * This is a tool to write SPI bootable images to a filesystem 
 * and build SPI images to a binary file for writing later.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef ___TOKENIZER_H__  
#define ___TOKENIZER_H__  

#ifdef WIN32
#pragma warning( disable : 4786)
#pragma warning( disable : 4267)
#pragma warning( disable : 4996)
#endif

#include <cassert>
#include <string>
#include <set>
#include <map>
#include <list>
#include <algorithm>
#include <stdio.h>
#include <string>

namespace util 
{

    template <class T> class base_stream 
    {
        public:
            virtual bool eof() const throw() = 0;
            virtual bool get_line(T & line, char delimiter) throw() = 0;
            virtual ~base_stream() throw() {}
    };

    template <class T> class file_stream : public base_stream<T> 
    {
        private:
            T _filename;
            FILE* _fstrm;

        public:
            enum { MAX_LINE_LEN = 256 };



            file_stream( const T & filename ) throw() : _filename(filename), _fstrm(0) { }


            bool open() throw() 
            {
                _fstrm = fopen( _filename.c_str(), "r" );

                return _fstrm ? true : false;
            }


            bool is_open() const throw() 
            { 
                return _fstrm != 0; 
            }


            bool close() throw() 
            {
                if (_fstrm) 
                {
                    bool ok = 0 == fclose(_fstrm);
                    _fstrm = 0;

                    return ok;
                }

                return true;
            }


            virtual bool eof() const throw()
            { 
                return feof(_fstrm) != 0;
            }


            bool getline( char * line_buf, int max_len, char delimiter ) 
            {
                int i = 0;
                char c = 0;

                while ( i < max_len ) 
                {
                    if (fread( &c, 1, 1, _fstrm ) != 1) 
                    {
                        return false;
                        break;
                    }

                    if ( c == delimiter ) 
                        break;

                    line_buf[i] = c;

                    i++;
                }

                return true;
            }


            virtual bool get_line(T & line, char delimiter) throw() 
            { 
                char lb[ MAX_LINE_LEN ] = { 0 };

                if ( getline( lb, sizeof(lb)-1, delimiter ) ) 
                {
                    line = lb;
                    return true;
                }

                return false;
            }


            virtual ~file_stream() throw() 
            {
                close();
            }
    };    


    template <class T> class tokenizer_t 
    {
        public:
            enum token_class_t 
            {
                BLANK            = 0, 
                ATOMIC,
                EMPTY_LINE,
                OTHER,
                END_OF_STREAM,
                LINESTYLE_COMMENT,

                TOKEN_CLASS_CNT
            };

            typedef std::set< T > token_class_set_t;

        private:
            base_stream<T> & _strm;

            int _current_line;
            int _current_col;

            T _current_line_buf;

            char _line_delimiter;

            token_class_set_t _token_class[ TOKEN_CLASS_CNT ];

        public:
            struct token_t 
            {
                token_class_t tkncls;
                T value;
                size_t col;
                size_t line;

                token_t() throw() : tkncls(OTHER), col(0), line(0) {}


                token_t(const token_t& obj) throw() 
                {
                    tkncls = obj.tkncls;
                    value = obj.value;
                    col = obj.col;
                    line = obj.line;
                }

                token_t & operator = (const token_t& obj) throw() 
                {
                    if ( this != &obj ) 
                    {
                        tkncls = obj.tkncls;
                        value = obj.value;
                        col = obj.col;
                        line = obj.line;
                    }

                    return *this;
                }


                bool operator == (const token_t& obj) const throw() 
                { 
                    return (this == &obj) || (tkncls == obj.tkncls && value == obj.value);
                }


                bool operator == (const T& str) const throw() { return value == str; }
                bool operator != (const T& str) const throw() { return value != str; }
            };

        private:
            token_t _last_processed_token;
            typedef std::list< token_t > _rtoken_list_t;
            bool _rtoken_enable;
            _rtoken_list_t _rtoken_list;
            typename _rtoken_list_t::const_iterator _rtoken_list_it;

            void _register_token_class( token_class_t cl, 
                    token_class_set_t clset ) throw()
            {
                assert( cl < TOKEN_CLASS_CNT && cl >= BLANK) ;
                _token_class [ cl ] = clset;
            }

        public:
            tokenizer_t( base_stream<T> & strm ) : 
                _strm(strm),
                _current_line( 0 ),
                _current_col( 0 ),
                _line_delimiter('\n'),
                _rtoken_enable(false),
                _rtoken_list(),
                _rtoken_list_it (_rtoken_list.begin()) {}    


            T get_current_line_buf() const throw() { return _current_line_buf; }


            void set_mark() throw() 
            { 
                _rtoken_enable = true;
                _rtoken_list.clear();
            }


            void remove_mark() throw() 
            { 
                _rtoken_enable = false;
                _rtoken_list.clear();
            }


            void rewind_to_mark() 
            {
                _rtoken_list_it = _rtoken_list.begin();
            }


            void register_token_blank( token_class_set_t clset ) throw() 
            {
                _register_token_class( BLANK, clset );
            }


            void register_token_atomic( token_class_set_t clset ) throw() 
            {
                _register_token_class( ATOMIC, clset );
            }

            void register_token_linestyle_comment( token_class_set_t clset ) throw() 
            {
                _register_token_class( LINESTYLE_COMMENT, clset );
            }

            int get_line_num() const throw() { return _current_line; }
            int get_col_num() const throw() { return _current_col; }
            bool eof() const throw() { return _strm.eof(); }
            void set_line_delimiter( char delimiter ) throw() { _line_delimiter = delimiter; }
            char get_line_delimiter() const throw() { return _line_delimiter; }


            static size_t search_token_pos(const T& token, 
                        const T& line, 
                        const size_t pos = 0 ) throw() 
            {
                return line.find( token, pos );
            }

            static size_t search_token_class_pos( T & token_found,
                        const token_class_set_t& tknset, 
                        const T & line,
                        const size_t pos = 0 )
            {
                int min_pos = unsigned(-1) >> 1;
                bool found = false;

                for ( typename token_class_set_t :: const_iterator i = tknset.begin();
                        i != tknset.end();
                        ++i )
                {
                    int ret_pos = -1;

                    if ( (ret_pos = (int) search_token_pos( *i, line, pos )) != -1 ) 
                    {
                        if ( ret_pos <= min_pos ) 
                        {
                            min_pos = ret_pos;
                            token_found = *i;
                            found = true;
                        }
                    }
                }

                return found ? min_pos : size_t(-1);
            }


        private:
            void _record_token( const token_t & t ) 
            {
                if ( _rtoken_enable ) 
                {
                    _rtoken_list.push_back( t );
                }
            }


            bool _replay_recorded_token( token_t & t ) 
            {
                if ( _rtoken_list_it != _rtoken_list.end()) 
                {

                    t = *_rtoken_list_it;
                    ++ _rtoken_list_it;

                    return true;
                }

                return false;
            }


        public:
            bool get_next_token( token_t & t ) throw() 
            {
                if ( _replay_recorded_token( t ) ) 
                {
                    return true;
                }

                t.tkncls = OTHER;

                //Read the text buffer one 
                if ( ! _current_col || _current_col == int(_current_line_buf.size()) ) 
                {
                    if (! _strm.get_line( _current_line_buf, _line_delimiter ) ) 
                    {
                        t.tkncls = END_OF_STREAM;
                        return false;
                    }
                    else 
                    {
                        ++ _current_line;
                        _current_col = 0;
                    }
                }

                T token;
                int atomic_token_pos = -1;
                int blank_pos = -1;
                int linestyle_comment = -1;

                do 
                {
                    atomic_token_pos = search_token_class_pos( 
                            token, 
                            _token_class[ ATOMIC ], 
                            _current_line_buf,
                            _current_col );


                    if ( atomic_token_pos == _current_col ) 
                    {
                        t.tkncls = ATOMIC;
                        break;
                    }

                    // search blank pos 
                    blank_pos = search_token_class_pos( token, 
                            _token_class[ BLANK ], 
                            _current_line_buf,
                            _current_col );

                    if ( blank_pos == _current_col ) 
                    {
                        t.tkncls = BLANK;
                        break;
                    } 

                    linestyle_comment = 
                        search_token_class_pos( token, 
                                _token_class[ LINESTYLE_COMMENT ], 
                                _current_line_buf,
                                _current_col );

                    if ( linestyle_comment == _current_col ) 
                    {
                        t.tkncls = LINESTYLE_COMMENT;

                        //left part of _current_line_buf is a token
                        token = _current_line_buf.substr( 
                                _current_col, 
                                _current_line_buf.size() - _current_col );

                        break;
                    }

                    int tpos = blank_pos > -1  && blank_pos < atomic_token_pos ? 
                        blank_pos : 
                        ( atomic_token_pos > -1 ? atomic_token_pos : blank_pos );


                    if ( tpos >= _current_col ) 
                    {
                        t.tkncls = OTHER;

                        //get token which precedes the blank
                        token = 
                            _current_line_buf.substr( _current_col, tpos - _current_col );
                    }
                    else 
                    {
                        //left part of _current_line_buf is a token
                        token = _current_line_buf.substr( 
                                _current_col, 
                                _current_line_buf.size() - _current_col );

                        break;
                    }
                }
                while (0);

                if ( token.empty() && _current_col == 0) 
                {
                    t.tkncls = EMPTY_LINE;
                }

                t.value = token;
                t.col = _current_col;
                t.line = _current_line;

                _last_processed_token = t;

                _current_col += token.size();

                _record_token( t );

                return ! token.empty() || t.tkncls == EMPTY_LINE;
            }


            token_t get_last_token_processed() const throw() 
            { 
                return _last_processed_token;
            }


    };
}
#endif

