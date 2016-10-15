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


//------------------------------------------------------------------------------

 
#ifdef WIN32
#include "targetver.h"
#include <tchar.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string>

#include "tokenizer.h"


//------------------------------------------------------------------------------


#define SPIDYBOOT_VERSION "1.0"


//------------------------------------------------------------------------------


class mc_config_t
{
    public:
        typedef unsigned int addr_t;
        typedef unsigned int value_t;

        typedef std::list< std::pair< addr_t, value_t> > assignlist_t;

        static void rebase_immr( addr_t base, addr_t newbase, assignlist_t & lst )
        {
            for ( assignlist_t::iterator i = lst.begin();
                    i != lst.end();
                    ++i )
            {
                if ((i->first & base) == base) 
                {
                    i->first ^= base;
                    i->first |= newbase;
                }
            }
        }

    private:
        typedef util::tokenizer_t< std::string > tokenizer_t;

        bool get_token( 
                tokenizer_t::token_t & token, 
                tokenizer_t & tknzr,   
                const tokenizer_t::token_class_t & skipping_cls = tokenizer_t::BLANK )
        {
            do 
            {
                if (! tknzr.get_next_token( token ) ) 
                {
                    return false;      
                }

            } while (token.tkncls == skipping_cls);  

            return true;
        }


        //--------------------------------------------------------------------------


        bool match ( const std::string& s, const std::list<std::string>& l ) 
        {
            std::list<std::string>::const_iterator i = l.begin();

            for (; i != l.end(); ++i ) {
                if ( s.find(*i) != std::string::npos ) {
                    return true;
                }
            }

            return false;
        }


        //--------------------------------------------------------------------------


        bool extr_exp_tkn( const std::string & expected_token, tokenizer_t & tknzr )
        {
            tokenizer_t::token_t token;

            if (!get_token(token, tknzr)) {
                return false;
            }

            if ( token != expected_token ) 
            {
                return false;
            }

            return true;
        }


        //--------------------------------------------------------------------------


        bool parse_cfg( tokenizer_t & tknzr, std::string& msg, assignlist_t& lst )
        {
            tokenizer_t::token_class_set_t blnk_cls;
            tokenizer_t::token_class_set_t sngt_cls;
            tokenizer_t::token_class_set_t linestyle_comment_cls;

            blnk_cls.insert(" ");
            blnk_cls.insert("\t");

            sngt_cls.insert("[");
            sngt_cls.insert("]");
            sngt_cls.insert(",");
            sngt_cls.insert("+");

            sngt_cls.insert("(");
            sngt_cls.insert(")");
            sngt_cls.insert("\"");
            sngt_cls.insert("=");

            linestyle_comment_cls.insert("//");
            linestyle_comment_cls.insert("#");

            tknzr.register_token_atomic( sngt_cls );
            tknzr.register_token_blank( blnk_cls );
            tknzr.register_token_linestyle_comment( linestyle_comment_cls );

            tokenizer_t::token_t token;
            bool end = false;
            bool syntax_error = false;

            while ( ! end ) 
            {
                token.value = "";

                end = ! tknzr.get_next_token( token );

                if ( token.tkncls == tokenizer_t::EMPTY_LINE ||
                        token.tkncls == tokenizer_t::LINESTYLE_COMMENT ||
                        token.tkncls == tokenizer_t::BLANK ) 
                {
                    //skip empty line or line-style comment
                    continue;
                }

                if ( token.tkncls == tokenizer_t::END_OF_STREAM ) 
                {
                    // no more token
                    break;
                }

                if ( token.value == "sleep" ) 
                {
                    std::string address = "0x40000001"; // TODO verify...
                    std::string value;

                    if ( ! get_token( token, tknzr ) ) {
                        msg = "value missing";
                        syntax_error = true;
                        break;
                    }

                    value = token.value;

                    addr_t ulAddr = 0;
                    value_t ulVal = 0;

                    sscanf(address.c_str(), "%x", &ulAddr);
                    sscanf(value.c_str(), "%x", &ulVal);

                    lst.push_back( std::make_pair< addr_t, value_t> ( ulAddr, ulVal ) );

                    continue;
                }

                if ( token.value == "writemem.l" ) 
                {
                    std::string address;
                    std::string value;

                    if ( ! get_token( token, tknzr ) ) 
                    {
                        msg = "first value missing";
                        syntax_error = true;
                        break;
                    }

                    address = token.value;

                    if (address.c_str()[0]<'0'|| address.c_str()[0]>'9')
                    {

                        msg = "invalid address ";
                        msg += address;
                        syntax_error = true;
                        break;
                    }

                    if ( ! get_token( token, tknzr ) ) 
                    {
                        msg = "second value missing";
                        syntax_error = true;
                        break;
                    }

                    value = token.value;

                    if (value.c_str()[0]<'0'|| value.c_str()[0]>'9')
                    {
                        msg = "invalid value ";
                        msg += value;
                        syntax_error = true;
                        break;
                    }

                    addr_t ulAddr = 0;
                    value_t ulVal = 0;

                    sscanf(address.c_str(), "%x", &ulAddr);
                    sscanf(value.c_str(), "%x", &ulVal);

                    lst.push_back( std::make_pair< addr_t, value_t> ( ulAddr, ulVal ) );

                    continue;
                }

                msg = "Unexpected symbol '";
                msg += token.value; 
                msg += "'";
                syntax_error = true;
                break;

            } // while

            return ! syntax_error;
        }


        //--------------------------------------------------------------------------


        bool parse_dat( tokenizer_t & tknzr, std::string& msg, assignlist_t& lst )
        {
            tokenizer_t::token_class_set_t blnk_cls;
            tokenizer_t::token_class_set_t sngt_cls;
            tokenizer_t::token_class_set_t linestyle_comment_cls;

            blnk_cls.insert(" ");
            blnk_cls.insert("\t");
            sngt_cls.insert(":");

            linestyle_comment_cls.insert("//");
            linestyle_comment_cls.insert("#");

            tknzr.register_token_atomic( sngt_cls );
            tknzr.register_token_blank( blnk_cls );
            tknzr.register_token_linestyle_comment( linestyle_comment_cls );

            tokenizer_t::token_t token;
            bool end = false;
            bool syntax_error = false;

            while ( ! end ) 
            {
                token.value = "";

                end = ! tknzr.get_next_token( token );

                if ( token.tkncls == tokenizer_t::EMPTY_LINE ||
                        token.tkncls == tokenizer_t::LINESTYLE_COMMENT ||
                        token.tkncls == tokenizer_t::BLANK ) 
                {
                    //skip empty line or line-style comment
                    continue;
                }

                if ( token.tkncls == tokenizer_t::END_OF_STREAM ) 
                {
                    // no more token
                    break;
                }

                std::string address;
                std::string value;

                address = token.value;

                if (!extr_exp_tkn(":", tknzr))
                {
                    msg = ": missing";
                    syntax_error = true;
                    break;
                }

                if ( ! get_token( token, tknzr ) ) 
                {
                    msg = "value missing";
                    syntax_error = true;
                    break;
                }

                value = token.value;

                addr_t ulAddr = 0;
                value_t ulVal = 0;

                sscanf(address.c_str(), "%x", &ulAddr);
                sscanf(value.c_str(), "%x", &ulVal);

                lst.push_back( std::make_pair< addr_t, value_t> ( ulAddr, ulVal ) );

            } // while

            return ! syntax_error;
        }


        //--------------------------------------------------------------------------


    public:


        //--------------------------------------------------------------------------


        bool compile_cfg( const std::string & filename, 
                assignlist_t& lst, 
                std::string& msg )
        {
            util::file_stream< std::string> fs( filename );
            tokenizer_t t( fs );

            if ( ! fs.open() ) 
            {
                msg = "Unable to open \"";
                msg += filename + "\"";
                return false;
            }

            bool res = parse_cfg( t, msg, lst );
            fs.close();

            return res;
        }


        //--------------------------------------------------------------------------


        bool compile_dat( const std::string & filename, 
                assignlist_t& lst, 
                std::string& msg )
        {
            util::file_stream< std::string> fs( filename );
            tokenizer_t t( fs );

            if ( ! fs.open() ) 
            {
                msg = "Unable to open \"";
                msg += filename + "\"";
                return false;
            }

            bool res = parse_dat( t, msg, lst );
            fs.close();

            return res;
        }

};


//------------------------------------------------------------------------------ 


class boot_spi_data_t
{
    /*
       SPI Bootable image format

       0x00-0x3F Reserved.

       0x40-0x43 BOOT signature. This location should contain the value 0x424F0_4F54, 
       which is the ASCII code for
       BOOT. The eSPI loader code will search for this signature, 
       initially in 24-bit addressable mode. 
       If the value in this location doesn't match the BOOT 
       signature, then the EEPROM is accessed again, but in 16-bit mode. 
       If the value in this location still does not match the BOOT signature, 
       it means that the eSPI device doesn't contain a valid user code. 
       In such case the eSPI loader code will disable the eSPI and will issue 
       a hardware reset request of the SoC by setting RSTCR[HRESET_REQ].

       0x44-0x47 Reserved

       0x48-0x4B User's code length. 
       Number of bytes in the user's code to be copied.
       Must be a multiple of 4.
       4 = User's code length = 2 Gbytes.

       0x4C-0x4F Reserved

       0x50-0x53 Source Address. Contains the starting address of the user's code 
       as an offset from the EEPROM starting address. In 24-bit addressing mode, 
       the 8 most significant bits of this should be written to as zero, because 
       the EEPROM is accessed with a 3-byte (24-bit) address. 
       In 16-bit addressing mode, the 16 most significant bits of this should 
       be written to as zero.

       0x54-0x57 Reserved

       0x58-0x5B Target Address. Contains the target address in the system's local 
       memory address space in which the user's code will be copied to. 
       This is a 32-bit effective address. The core is configured in such a way 
       that the 36-bit real address is equal to this (with 4 most significant bits 
       zero).

       0x5C-0x5F Reserved

       0x60-0x63 Execution Starting Address. Contains the jump address in the 
       system's local memory address space into the user's code first instruction 
       to be executed. 
       This is a 32-bit effective address. The core is configured in such a way 
       that the 36-bit real address is equal to this (with 4 most significant bits 
       zero).

       0x64-0x67 Reserved

       0x68-0x6B N. Number of Config Address/Data pairs. 

       0x6C-0x7F Reserved.
       0x80-0x83 Config Address 1
       0x84-0x87 Config Data 1
       0x88-0x8B Config Address 2
       0x8C-0x8F Config Data 2
       ...
       0x80 + 8x(N-1) Config Address N
       0x80 + 8x(N-1) + 4 Config Data N (Final Config Data N optional)
       ...
       User's Code
     */

    private:

        // Default pre-initialized preable 
        static unsigned char preamble_bin[ 108 ];


        //--------------------------------------------------------------------------


        unsigned char _data[ 1024 ];


        //--------------------------------------------------------------------------


        enum 
        {
            OFS_USER_CODE_LEN = 0x48,
            OFS_SRC_ADDR      = 0x50,
            OFS_TARGET_ADDR   = 0x58,
            OFS_EXEST_ADDR    = 0x60,
            OFS_CFG_PAIRS_NUM = 0x68,
            OFS_FIRST_CFG_ADDR= 0x80,
            OFS_FIRST_CFG_DATA= 0x84
        };


        //--------------------------------------------------------------------------


    public:


        boot_spi_data_t() 
        {
            memset( _data, 0, sizeof(_data) );
        }


        //--------------------------------------------------------------------------


        void set_default()
        {
            memcpy( _data, preamble_bin, sizeof(preamble_bin) );
        }


        //--------------------------------------------------------------------------


        bool load_from_file( const std::string& filename )
        {
            FILE * f = fopen( filename.c_str(), "rb" );

            if (f) {
                int rb = fread( _data, 1, sizeof(_data), f );
                if (rb<sizeof(_data))
                {
                    fclose(f);
                    return false;
                }
                fclose(f);
                return true;
            }
            return false;
        }


        //--------------------------------------------------------------------------


        bool save( const std::string& filename )
        {
            FILE * f = fopen( filename.c_str(), "wb" );

            if (!f) 
            {
                return false;
            }

            int rb = fwrite( _data, 1, sizeof(_data), f );

            if (rb< ((int) sizeof(_data)))
            {
                fclose(f);
                return false;
            }

            fclose(f);
            return true;
        }


        //--------------------------------------------------------------------------


        bool patch( const std::string& filename )
        {
            FILE * f = fopen( filename.c_str(), "r+b" );

            if (!f)
            {
                return false;
            }

            if (fseek(f, 0, SEEK_END)<0)
            {
                fclose(f);
                return false;
            }

            size_t size = ftell(f);
            fseek(f, 0, SEEK_SET);

            if (int(size) < ( (int) sizeof(_data) ) )
            {
                fclose(f);
                return false;
            }

            int rb = fwrite( _data, 1, sizeof(_data), f );

            if (rb< ((int) sizeof(_data)))
            {
                fclose(f);
                return false;
            }

            fclose(f);
            return true;
        }


        //--------------------------------------------------------------------------


        bool attach_to( const std::string& srcname, const std::string& dstname )
        {
            bool ret = false;
            FILE * f = 0;
            char * databuf = 0;

            do {
                //open source file
                f = fopen( srcname.c_str(), "rb" );

                //get file size
                if (!f) break;
                if (0>fseek( f, 0, SEEK_END )) break;
                int len = (int) ftell( f );
                if (len <0) break;
                fseek(f, 0, SEEK_SET);

                //allocate buffer of file size + header len bytes
                databuf = new char [ len + sizeof(_data) ];
                if (! databuf)  break; // no enough memory 

                //set preamble
                memcpy(databuf, _data, sizeof(_data));

                //append content of source file
                int rb = fread( databuf+sizeof(_data), 1, len, f );
                if (rb<len) break;

                fclose(f);

                //create destination file
                f = fopen( dstname.c_str(), "wb" );
                if (!f) break; 

                //write preamble + data
                int wb = fwrite( databuf, 1, len + sizeof(_data), f );
                if (wb<(len + (int) sizeof(_data)))  break;

                ret = true; // terminated succesfully
            }
            while(0);

            if (f) fclose(f);
            delete [] databuf;
            return ret;
        }


        //--------------------------------------------------------------------------


        inline unsigned int get_dword(int offset) const throw()
        {
            unsigned int value = 
                (_data[offset + 0]<<24) + 
                (_data[offset + 1]<<16) + 
                (_data[offset + 2]<<8)  +  
                (_data[offset + 3]);

            return value;
        }


        //--------------------------------------------------------------------------


        inline void patch_dword_at(int offset, unsigned int data)
        {
            _data[offset + 0] = (data>>24) & 0xff;
            _data[offset + 1] = (data>>16) & 0xff;
            _data[offset + 2] = (data>> 8) & 0xff;
            _data[offset + 3] = (data>> 0) & 0xff;
        }


        //--------------------------------------------------------------------------


        inline unsigned int get_user_code_len() const throw()
        {
            return get_dword( OFS_USER_CODE_LEN );
        }


        //--------------------------------------------------------------------------


        inline void set_user_code_len( unsigned int data ) throw()
        {
            patch_dword_at( OFS_USER_CODE_LEN, data );
        }


        //--------------------------------------------------------------------------


        inline unsigned int get_src_addr() const throw()
        {
            return get_dword( OFS_SRC_ADDR );
        }


        //--------------------------------------------------------------------------


        inline void set_src_addr( unsigned int data ) throw()
        {
            patch_dword_at( OFS_SRC_ADDR, data );
        }


        //--------------------------------------------------------------------------


        inline unsigned int get_target_addr() const throw()
        {
            return get_dword( OFS_TARGET_ADDR );
        }


        //--------------------------------------------------------------------------


        inline void set_target_addr( unsigned int data ) throw()
        {
            patch_dword_at( OFS_TARGET_ADDR, data );
        }


        //--------------------------------------------------------------------------


        inline unsigned int get_exest_addr() const throw()
        {
            return get_dword( OFS_EXEST_ADDR );
        }


        //--------------------------------------------------------------------------


        inline void set_exest_addr( unsigned int data ) throw()
        {
            patch_dword_at( OFS_EXEST_ADDR, data );
        }


        //--------------------------------------------------------------------------


        inline unsigned int get_n_cfg_pairs() const throw()
        {
            return get_dword( OFS_CFG_PAIRS_NUM );
        }


        //--------------------------------------------------------------------------


        inline void set_n_cfg_pairs( unsigned int data ) throw()
        {
            patch_dword_at( OFS_CFG_PAIRS_NUM, data );
        }


        //--------------------------------------------------------------------------


        bool set_cfg_pair( int idx, unsigned int addr, unsigned int data ) throw()
        {
            int dataofs_addr = OFS_FIRST_CFG_ADDR + (idx<<3);
            int dataofs_data = OFS_FIRST_CFG_DATA + (idx<<3);

            if (dataofs_addr >= (sizeof(_data)) )
            {
                return false;
            }

            patch_dword_at( dataofs_addr, addr );
            patch_dword_at( dataofs_data, data );

            return true;
        }


        //--------------------------------------------------------------------------


        void show() const throw()
        {
            char boot_sign[ 5 ] = {0};
            boot_sign[0] = _data[0x40];
            boot_sign[1] = _data[0x41];
            boot_sign[2] = _data[0x42];
            boot_sign[3] = _data[0x43];

            bool valid_sign = std::string(boot_sign) == "BOOT";

            printf(" 0x40- 0x43 BOOT signature      :  0x%02x%02x%02x%02x == "
                    "'%s' %s\n", 
                    boot_sign[0], boot_sign[1], boot_sign[2], boot_sign[3], boot_sign,
                    valid_sign ? "OK" : "NOT OK");

            if (! valid_sign )
            {
                printf("WARNING: inalid signature '%s' != 'BOOT'..."
                        "wrong header format ?\n", boot_sign);
            }

            unsigned int val = get_user_code_len();

            printf(" 0x48- 0x4B User's code length  :  0x%08x (%u bytes - %u Kb)\n", 
                    val, val, (val >> 10) + ((val & 1023) ? 1 : 0 ));

            if ((unsigned) val> 1024U*1024U )
            {
                printf("WARNING: code length seems uge... wrong header format ?\n");
            }

            printf(" 0x50- 0x53 Source Address      :  0x%08x\n", get_src_addr());
            printf(" 0x58- 0x5B Target Address      :  0x%08x\n", get_target_addr());
            printf(" 0x60- 0x63 Exe Start Address   :  0x%08x\n", get_exest_addr());

            val = get_n_cfg_pairs();

            printf(" 0x68- 0x6B N.of Adr/Data pairs :  0x%08x (%u)\n", val, val);

            if ((unsigned) val> 1024U )
            {
                printf("WARNING: too many args, cutting off at 1024 bytes\n");
            }

            for ( int i = 0; i<int(val); ++i )
            {
                int dataofs_addr = OFS_FIRST_CFG_ADDR + (i<<3);
                int dataofs_data = OFS_FIRST_CFG_DATA + (i<<3);

                if (dataofs_addr >= (sizeof(_data)) )
                {
                    break;
                }

                unsigned int addr = get_dword( dataofs_addr );
                unsigned int data = get_dword( dataofs_data );

                printf("0x%03x-0x%03x addr[%2i]@0x%08x := 0x%08x\n", 
                        dataofs_addr, dataofs_data+3, i, addr, data);
            }
        }

        //------------------------------------------------------------------------------


};


class cmd_args_t
{
    public:

        struct cfg_t
        {
            std::string app_fname;
            bool show_help;
            bool show_version;
            bool show_info;
            bool rebase;
            bool replacepreamble;
            bool patchtrgaddr;
            bool patchsrcaddr;
            bool patchexeaddr;

            mc_config_t::addr_t baddr;
            mc_config_t::addr_t newaddr;
            mc_config_t::addr_t trgaddr;
            mc_config_t::addr_t srcaddr;
            mc_config_t::addr_t exeaddr;

            std::string error;
            std::string bin_fname;
            std::string prb_fname;
            std::string cfg_fname;
            std::string dat_fname;
            std::string src_fname;
            std::string dst_fname;


            //--------------------------------------------------------------------------


            inline cfg_t() throw() 
                :
                    app_fname("spidyboot"),
                    show_help(false),
                    show_version(false),
                    show_info(false),
                    rebase(false),
                    replacepreamble(false),
                    patchtrgaddr(false),
                    patchsrcaddr(false),
                    patchexeaddr(false),
                    baddr(0),
                    newaddr(0),
                    trgaddr(0),
                    srcaddr(0),
                    exeaddr(0)
            {}
        }
        config;


        //------------------------------------------------------------------------------


        void show_help() const throw()
        {
            printf("Usage:\n");
            printf("%s \n"
                    "   --help |\n"
                    "   --ver  |\n"
                    "   --show \n"
                    "   --bin <src_binary_file> \n"
                    "   --cfg <cfg_file> |  --dat <dat_file> \n"
                    " [ --prb <preamble_file> ] \n"
                    " [ --spi -s <bootcode_file> -d <spiboot_file> | "
                    "--patch <spiboot_file> ] \n"
                    " [ --addr <baddr> <newaddr> ]\n"
                    " [ --tga <trgaddr> ] \n"
                    " [ --sra <srcaddr> ] \n"
                    " [ --exe <exeaddr> ] \n",
                    config.app_fname.c_str());

            printf("Where:\n--help\n");
            printf("  Show this help message\n\n");

            printf("--ver\n");
            printf("  Show program version\n\n");

            printf("--show\n");
            printf("  Show preamble info\n\n");

            printf("--bin <src_binary_file>\n");
            printf("  Read the preamble from <src_binary_file>\n\n");

            printf("--cfg <cfg_file>\n");
            printf("  Modify the preamble by using "
                    "data read from a DRAM config file\n\n");

            printf("--dat <dat_file>\n");
            printf("  Modify the preamble by using "
                    "data read from a DAT file\n\n");

            printf("--prb <preamble_file> \n");
            printf("  Save the preamble in the file <preamble_file>\n\n");

            printf("--spi -s <bootcode_file> -d <spiboot_file> \n");
            printf("  Create a spi-flash image: "
                    "<spiboot_file> = preamble + <bootcode_file>\n\n");

            printf("--patch <spiboot_file>\n");
            printf("  Patch the preamble of an existing spi-flash image\n\n");

            printf("--addr <baddr> <newaddr>\n");
            printf("  Replace the base address <baddr> with new value <newaddr>\n\n");

            printf("--tga <trgaddr> \n");
            printf("  Replace the default target address with new value <trgaddr>\n\n");

            printf("--sra <srcaddr> \n");
            printf("  Replace the default source address with new value <srcaddr>\n\n");

            printf("--exe <exeaddr> \n");
            printf("  Replace the default exe start address with new value <exeaddr>\n");
        }

        void show_version() const throw()
        {
            printf("Version " SPIDYBOOT_VERSION " \n");
        }


    private:

        enum state_t
        {
            CONTINUE_PARSING,
            GET_BINFILE,
            GET_CFGFILE,
            GET_DATFILE,
            GET_PBLFILE,
            GET_SRCDSTPARAM,
            GET_SRCPARAM,
            GET_SRCFILE,
            GET_DSTPARAM,
            GET_DSTFILE,
            GET_SPIFILE,
            GET_BADDR,
            GET_NEWADDR,
            GET_SRCADDR,
            GET_TRGADDR,
            GET_EXEADDR
        };

    public:
        cmd_args_t( int argc, char* argv[] ) throw()
        {
            state_t s = CONTINUE_PARSING;

            for (int i = 0;
                    i<argc;
                    ++i)
            {
                if (i==0)
                {
                    config.app_fname = argv[0];
                    continue;
                }

                std::string sArg = argv[ i ];

                if (s == CONTINUE_PARSING && sArg == "--help") 
                {
                    config.show_help = true;

                    if (argc != 2 )
                    {
                        config.error = "syntax error";
                    }

                    break;
                }
                else if (s == CONTINUE_PARSING && sArg == "--ver") 
                {
                    config.show_version = true;

                    if (argc != 2 )
                    {
                        config.error = "syntax error";
                    }

                    break;
                }
                else if (s == CONTINUE_PARSING && sArg == "--show") 
                {
                    config.show_info = true;
                }
                else if (s == CONTINUE_PARSING && sArg == "--bin" )
                {
                    s = GET_BINFILE;
                }
                else if (s == GET_BINFILE )
                {
                    config.bin_fname = sArg;

                    s = CONTINUE_PARSING;
                }
                else if (s == CONTINUE_PARSING && sArg == "--cfg" )
                {
                    s = GET_CFGFILE;
                }
                else if (s == CONTINUE_PARSING && sArg == "--dat" )
                {
                    s = GET_DATFILE;
                }
                else if (s == GET_CFGFILE )
                {
                    config.cfg_fname = sArg;
                    s = CONTINUE_PARSING;
                }
                else if (s == GET_DATFILE )
                {
                    config.dat_fname = sArg;
                    s = CONTINUE_PARSING;
                }
                else if (s == CONTINUE_PARSING && sArg == "--prb" )
                {
                    s = GET_PBLFILE;
                }
                else if (s == GET_PBLFILE )
                {
                    config.prb_fname = sArg;
                    s = CONTINUE_PARSING;
                }
                else if (s == CONTINUE_PARSING && sArg == "--spi" )
                {
                    s = GET_SRCDSTPARAM;
                }
                else if ((s == GET_SRCDSTPARAM || s==GET_SRCPARAM) && sArg == "-s" )
                {
                    s = GET_SRCFILE;
                }
                else if ((s == GET_SRCDSTPARAM || s==GET_DSTPARAM) && sArg == "-d" )
                {
                    s = GET_DSTFILE;
                }
                else if ((s == CONTINUE_PARSING) && sArg == "--patch" )
                {
                    s = GET_SPIFILE;
                }
                else if (s == GET_SRCFILE )
                {
                    config.src_fname = sArg;
                    s = config.dst_fname.empty() ? GET_DSTPARAM : CONTINUE_PARSING;
                }
                else if (s == GET_DSTFILE )
                {
                    config.dst_fname = sArg;
                    s = config.src_fname.empty() ? GET_SRCPARAM : CONTINUE_PARSING;
                }
                else if (s == GET_SPIFILE )
                {
                    config.dst_fname = sArg;
                    config.replacepreamble = true;
                    s = CONTINUE_PARSING;
                }
                else if (s == CONTINUE_PARSING && sArg == "--addr" )
                {
                    s = GET_BADDR;
                }
                else if (s == CONTINUE_PARSING && sArg == "--tga" )
                {
                    s = GET_TRGADDR;
                }
                else if (s == CONTINUE_PARSING && sArg == "--sra" )
                {
                    s = GET_SRCADDR;
                }
                else if (s == CONTINUE_PARSING && sArg == "--exe" )
                {
                    s = GET_EXEADDR;
                }
                else if (s == GET_BADDR )
                {
                    unsigned int addr = 0;
                    sscanf( sArg.c_str(), "%x",  &addr );
                    config.baddr = addr;

                    s = GET_NEWADDR;
                }
                else if (s == GET_NEWADDR )
                {
                    unsigned int addr = 0;
                    sscanf( sArg.c_str(), "%x",  &addr );
                    config.newaddr = addr;

                    config.rebase = true;

                    s = CONTINUE_PARSING;
                }
                else if (s == GET_TRGADDR )
                {
                    unsigned int addr = 0;
                    sscanf( sArg.c_str(), "%x",  &addr );
                    config.trgaddr = addr;

                    config.patchtrgaddr = true;

                    s = CONTINUE_PARSING;
                }
                else if (s == GET_SRCADDR )
                {
                    unsigned int addr = 0;
                    sscanf( sArg.c_str(), "%x",  &addr );
                    config.srcaddr = addr;

                    config.patchsrcaddr = true;

                    s = CONTINUE_PARSING;
                }
                else if (s == GET_EXEADDR )
                {
                    unsigned int addr = 0;
                    sscanf( sArg.c_str(), "%x",  &addr );
                    config.exeaddr = addr;

                    config.patchexeaddr = true;

                    s = CONTINUE_PARSING;
                }
                else //ERROR
                { 
                    config.error = std::string("'") + sArg + "' syntax error";

                    break; // error
                }

            } // for

            switch (s)
            {
                case GET_CFGFILE:
                    config.error = "Missing <cfg_file> argument";
                    break;

                case GET_DATFILE:
                    config.error = "Missing <dat_file> argument";
                    break;

                case GET_PBLFILE:
                    config.error = "Missing <preamble_file> argument";
                    break;

                case GET_BADDR:
                    config.error = "Missing <baddr> and <newaddr> arguments";
                    break;

                case GET_NEWADDR:
                    config.error = "Missing <newaddr> argument";
                    break;

                case GET_TRGADDR:
                    config.error = "Missing <trgaddr> argument";
                    break;

                case GET_SRCADDR:
                    config.error = "Missing <srcaddr> argument";
                    break;

                case GET_EXEADDR:
                    config.error = "Missing <exeaddr> argument";
                    break;

                case GET_SRCDSTPARAM:
                    config.error = 
                        "Missing -s <bootcode_file> or -d <spiboot_file> argument";
                    break;

                case GET_SRCPARAM:
                    config.error = "Missing -s <bootcode_file> argument";
                    break;

                case GET_DSTPARAM:
                    config.error = "Missing -d <spiboot_file> argument";
                    break;

                default:
                    if (s != CONTINUE_PARSING)
                    {
                        if (config.error.empty())
                            config.error = "Syntax error";
                    }
                    break;

            } // switch     
        }
};

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#ifdef WIN32
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{
//////////////////////////////////////////////////////////////////////////////
// Parse the command line
//
    cmd_args_t args(argc, argv);

    if (! args.config.error.empty())
    {
        fprintf(stderr, "%s\n", args.config.error.c_str());
        return 1;
    }

    if (argc<2 || args.config.show_help)
    {
        args.show_help();
        return 0;
    }

    if (args.config.show_version)
    {
        args.show_version();
        return 0;
    }

    boot_spi_data_t boot_spi_data;


//////////////////////////////////////////////////////////////////////////////
// Process bynary file (--bin)
//
    if (! args.config.bin_fname.empty())
    {
        if (! boot_spi_data.load_from_file( args.config.bin_fname ) )
        {
            perror("Error loading file");
            return 1;
        }
    }
    else
    {
        boot_spi_data.set_default();
    }


//////////////////////////////////////////////////////////////////////////////
// Process cfg file (--cfg)
//
    mc_config_t::assignlist_t lst;

    if (! args.config.cfg_fname.empty())
    {
        mc_config_t cfg;
        std::string msg;

        if (! cfg.compile_cfg( args.config.cfg_fname, lst, msg) )
        {
            if (msg.empty())
            {
                fprintf(stderr, "Cannot compile file '%s'\n", 
                        args.config.cfg_fname.c_str());
            }
            else
            {
                fprintf(stderr, "Error compiling \"%s\" : '%s'\n", 
                        args.config.cfg_fname.c_str(),
                        msg.c_str());
            }

            return 1;
        }
    }


//////////////////////////////////////////////////////////////////////////////
// Process dat file (--dat)
//
    mc_config_t::assignlist_t datlst;

    if (! args.config.dat_fname.empty())
    {
        mc_config_t cfg;
        std::string msg;

        if (! cfg.compile_dat( args.config.dat_fname, datlst, msg) )
        {
            if (msg.empty())
            {
                fprintf(stderr, "Cannot compile file '%s'\n", 
                        args.config.dat_fname.c_str());
            }
            else
            {
                fprintf(stderr, "Error compiling \"%s\" : '%s'\n", 
                        args.config.dat_fname.c_str(),
                        msg.c_str());
            }

            return 1;
        }
    }


//////////////////////////////////////////////////////////////////////////////
// Rebase address (--addr)
//
    if (args.config.rebase)
    {
        mc_config_t::rebase_immr( args.config.baddr, args.config.newaddr, lst );
    }


//////////////////////////////////////////////////////////////////////////////
// Patch preamble 
//

    //--tga
    if (args.config.patchtrgaddr)
    {
        boot_spi_data.set_target_addr( args.config.trgaddr );
    }

    //--sra
    if (args.config.patchsrcaddr)
    {
        boot_spi_data.set_src_addr( args.config.srcaddr );
    }

    //--exe
    if (args.config.patchexeaddr)
    {
        boot_spi_data.set_exest_addr( args.config.exeaddr );
    }

    // process .cfg patch list
    if (!lst.empty())
    {
        boot_spi_data.set_n_cfg_pairs( lst.size() );

        int idx = 0;

        for (mc_config_t::assignlist_t::const_iterator i = lst.begin();
                i != lst.end();
                ++i, ++idx)
        {
            boot_spi_data.set_cfg_pair( idx, i->first, i->second );
        }
    }

    // process .dat patch list
    if (!datlst.empty())
    {
        for (mc_config_t::assignlist_t::const_iterator i = datlst.begin();
                i != datlst.end();
                ++i)
        {
            boot_spi_data.patch_dword_at( i->first, i->second );
        }
    }


//////////////////////////////////////////////////////////////////////////////
// Modify the preamble of an existing spi-flash boot image (--patch)
//
    if ( args.config.replacepreamble && ! args.config.dst_fname.empty() )
    {
        if (! boot_spi_data.patch( args.config.dst_fname ))
        {
            perror("Error patching spi-flash image file");
            return 1;
        }
    }


//////////////////////////////////////////////////////////////////////////////
// Merge preamble and source boot image in order to create 
// a new spi-flash boot image (--spi)
//
    if ( ! args.config.replacepreamble && 
            ! args.config.dst_fname.empty() &&
            ! args.config.src_fname.empty() )
    {
        if (! boot_spi_data.attach_to( args.config.src_fname, 
                    args.config.dst_fname ))
        {
            perror("Error creating spi-flash image file");
            return 1;
        }
    }


//////////////////////////////////////////////////////////////////////////////
// Create a new preable binary file (--prb)
//
    if ( ! args.config.prb_fname.empty() )
    {
        if (! boot_spi_data.save( args.config.prb_fname ))
        {
            perror("Error creating preamble file");
            return 1;
        }
    }


//////////////////////////////////////////////////////////////////////////////
// Print out the preamble info (--show)
//
    if ( args.config.show_info )
    {
        boot_spi_data.show();
    }

    return 0;
}


//------------------------------------------------------------------------------


//::::::::::::::::::::::::::::::: boot_spi_data_t ::::::::::::::::::::::::::::::

unsigned char boot_spi_data_t::preamble_bin[ 108 ] = 
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x42, 0x4F, 0x4F, 0x54, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x11, 0x07, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
};


