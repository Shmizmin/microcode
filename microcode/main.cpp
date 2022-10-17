#include <cstdint>
#include <cstdlib>
#include <cstdio>

#include <fstream>
#include <array>
#include <string>
#include <string_view>
#include <limits>

#include "opcode.h"

namespace
{
    consteval auto BIT(unsigned int s) noexcept
    {
        return (std::uint64_t{ 1 } << s);
    }
    
    consteval auto LEN(unsigned int s) noexcept
    {
        // TODO: this is a bit hacky
        return (std::uint64_t{ s & 2 } << 62);
    }
    
    static constexpr const auto µcode_line_width = 8;
    
    using µcode_type = std::uint64_t;
    using µcode_line = std::array<µcode_type, µcode_line_width>;
    
    enum : µcode_type
    {
        // register file
        RF_AI = BIT(0),
        RF_BI = BIT(1),
        RF_CI = BIT(2),
        RF_DI = BIT(3),
        RF_FI = BIT(4),
        
        RF_AO = BIT(5),
        RF_BO = BIT(6),
        RF_CO = BIT(7),
        RF_DO = BIT(8),
        RF_FO = BIT(9),
        // /register file
        
        // load store unit
        LSU_RE = BIT(10),
        LSU_WE = BIT(11),
        LSU_SP_D = BIT(12), //stack * direction, 1 = decrement
        LSU_SP_WE = BIT(13),
        LSU_SP_EN = BIT(14),
        // /load store unit
        
        // arithmetic logic unit
        ALU_ADD = BIT(15),
        ALU_SUB = BIT(16),
        ALU_AND = BIT(17),
        ALU_OR = BIT(18),
        ALU_NOT = BIT(19),
        ALU_SHL = BIT(20),
        ALU_SHR = BIT(21),
        ALU_WA = BIT(22),
        ALU_WB = BIT(23),
        ALU_OE = BIT(24),
        // /arithmetic logic unit
        
        // instruction register
        IR_WE = BIT(25),
        // /instruction register
        
        // program counter
        PC_LRC = BIT(26),
        PC_INI = BIT(27),
        PC_CUB = BIT(28),
        PC_OE = BIT(29),
        // /program counter
        
        // address composition unit
        ACU_WL = BIT(30),
        ACU_WH = BIT(31),
        ACU_OE = BIT(32),
        // /address composition unit
        
        // address decomposition unit
        ADU_RL = BIT(33),
        ADU_RH = BIT(34),
        ADU_WE = BIT(35),
        // /address decomposition unit
        
        // misc
        FORCE_JUMP = BIT(54),
        REQUEST_JEZ = BIT(55),
        REQUEST_JCS = BIT(56),
        CONNECT_FB = BIT(57),
        OUT_Q1 = BIT(58),
        OUT_Q2 = BIT(59),
        SET_HALT = BIT(60),
        // /misc
    };
    
    constexpr const auto FETCH_INSTRUCTION = PC_OE | LSU_RE | IR_WE,
                                LSU_SP_INC = LSU_SP_EN | LSU_SP_WE,
                                LSU_SP_DEC = LSU_SP_EN | LSU_SP_WE | LSU_SP_D,
                            LSU_READ_STACK = LSU_SP_EN | LSU_RE,
                           LSU_WRITE_STACK = LSU_SP_EN | LSU_WE;
    
    // TODO: determine how the instruction register will be written to after a reset to begin
    
    
    //maybe--on reset
    // PC_OE | PC_LRC | LSU_RE | IR_WE
    
    // Pseudocode:
    // output pc
    // input pc as load & reset -> step = 0 again (hopefully)
    // read from rom
    // write to ir
    
    consteval auto chk_trap(bool trap) noexcept
    {
        return trap ? SET_HALT : 0;
    }
    
    consteval µcode_line emit_mvb(µcode_type dest_in, µcode_type src_out, bool trap, bool fmode) noexcept
    {
        return
        {
            LEN(1) | FETCH_INSTRUCTION,
            LEN(1) | src_out | dest_in | (fmode ? CONNECT_FB : 0),
            LEN(1) | PC_INI | chk_trap(trap),
            
            0ull, 0ull, 0ull, 0ull, 0ull,
        };
    }
    
    consteval µcode_line emit_alu2reg(µcode_type dest_in, µcode_type dest_out, µcode_type src_out, µcode_type op, bool trap) noexcept
    {
        return
        {
            LEN(1) | FETCH_INSTRUCTION,
            LEN(1) | dest_out | ALU_WA,
            LEN(1) | src_out | ALU_WB,
            LEN(1) | op | ALU_OE | dest_in | RF_FI,
            LEN(1) | PC_INI | chk_trap(trap),
            
            0ull, 0ull, 0ull,
        };
    }
    
    consteval µcode_line emit_aluimm(µcode_type dest_in, µcode_type dest_out, µcode_type op, bool trap) noexcept
    {
        return
        {
            LEN(2) | FETCH_INSTRUCTION,
            LEN(2) | dest_out | ALU_WA,
            LEN(2) | OUT_Q1 | ALU_WB,
            LEN(2) | op | ALU_OE | dest_in | RF_FI,
            LEN(2) | PC_INI | chk_trap(trap),
            
            0ull, 0ull, 0ull,
        };
    }
    
    consteval µcode_line emit_alunot(µcode_type dest_in, µcode_type dest_out, bool trap) noexcept
    {
        return
        {
            LEN(1) | FETCH_INSTRUCTION,
            LEN(1) | dest_out | ALU_WA,
            LEN(1) | ALU_NOT | ALU_OE | dest_in | RF_FI,
            LEN(1) | PC_INI | chk_trap(trap),
            
            0ull, 0ull, 0ull, 0ull,
        };
    }
    
    consteval µcode_line emit_nop(bool trap) noexcept
    {
        return
        {
            LEN(1) | FETCH_INSTRUCTION,
            LEN(1) | PC_INI | chk_trap(trap),
            
            0ull, 0ull, 0ull, 0ull, 0ull, 0ull,
        };
    }
    
    //doesn't accept trapping
    consteval µcode_line emit_brk(void) noexcept
    {
        return
        {
            LEN(1) | FETCH_INSTRUCTION,
            LEN(1) | PC_INI | SET_HALT,
            
            0ull, 0ull, 0ull, 0ull, 0ull, 0ull,
        };
    }
    
    consteval µcode_line emit_ldbimm(µcode_type dest_in, bool trap) noexcept
    {
        return
        {
            LEN(2) | FETCH_INSTRUCTION,
            LEN(2) | OUT_Q1 | dest_in,
            LEN(2) | PC_INI | chk_trap(trap),
            
            0ull, 0ull, 0ull, 0ull, 0ull,
        };
    }
    
    consteval µcode_line emit_ldbmem(µcode_type dest_in, bool trap) noexcept
    {
        return
        {
            LEN(3) | FETCH_INSTRUCTION,
            LEN(3) | OUT_Q1 | ACU_WL,
            LEN(3) | OUT_Q2 | ACU_WH,
            LEN(3) | ACU_OE | LSU_RE | dest_in,
            LEN(3) | PC_INI | chk_trap(trap),
            
            0ull, 0ull, 0ull,
        };
    }
    
    consteval µcode_line emit_stbmem(µcode_type src_out, bool trap) noexcept
    {
        return
        {
            LEN(3) | FETCH_INSTRUCTION,
            LEN(3) | OUT_Q1 | ACU_WL,
            LEN(3) | OUT_Q2 | ACU_WH,
            LEN(3) | ACU_OE | LSU_WE | src_out,
            LEN(3) | PC_INI | chk_trap(trap),
            
            0ull, 0ull, 0ull,
        };
    }
    
    consteval µcode_line emit_pop8r(µcode_type dest_in, bool trap) noexcept
    {
        return
        {
            LEN(1) | FETCH_INSTRUCTION,
            LEN(1) | LSU_READ_STACK | dest_in,
            LEN(1) | LSU_SP_INC,
            LEN(1) | PC_INI | chk_trap(trap),
            
            0ull, 0ull, 0ull, 0ull,
        };
    }
    
    consteval µcode_line emit_push8r(µcode_type src_out, bool trap) noexcept
    {
        return
        {
            LEN(1) | FETCH_INSTRUCTION,
            LEN(1) | LSU_SP_DEC,
            LEN(1) | LSU_WRITE_STACK | src_out,
            LEN(1) | PC_INI | chk_trap(trap),
            
            0ull, 0ull, 0ull, 0ull,
        };
    }
    

    consteval µcode_line emit_popip(bool trap) noexcept
    {
        return
        {
            LEN(1) | FETCH_INSTRUCTION,
            LEN(1) | LSU_READ_STACK | ACU_WL,
            LEN(1) | LSU_SP_INC,
            LEN(1) | LSU_READ_STACK | ACU_WH,
            LEN(1) | LSU_SP_INC,
            LEN(1) | ACU_OE | PC_LRC | FORCE_JUMP,
            LEN(1) | PC_INI | chk_trap(trap),
            
            0ull,
        };
    }
    
    consteval µcode_line emit_puship(bool trap) noexcept
    {
        return
        {
            LEN(1) | FETCH_INSTRUCTION,
            LEN(1) | PC_OE | ADU_WE,
            LEN(1) | LSU_SP_DEC,
            LEN(1) | LSU_WRITE_STACK | ADU_RH,
            LEN(1) | LSU_SP_DEC,
            LEN(1) | LSU_WRITE_STACK | ADU_RL,
            LEN(1) | PC_INI | chk_trap(trap),
            
            0ull,
        };
    }
    
    consteval µcode_line emit_jump(µcode_type condition, bool trap) noexcept
    {
        return
        {
            LEN(1) | FETCH_INSTRUCTION,
            LEN(1) | OUT_Q1 | ACU_WL,
            LEN(1) | OUT_Q2 | ACU_WH,
            LEN(1) | ACU_OE | PC_LRC | condition | chk_trap(trap),
            
            0ull, 0ull, 0ull, 0ull,
        };
    }
    
    consteval µcode_line emit_deref(µcode_type src1_out, µcode_type src2_out, µcode_type dest_in, bool trap)
    {
        return
        {
            LEN(1) | FETCH_INSTRUCTION,
            LEN(1) | src1_out | ACU_WL,
            LEN(1) | src2_out | ACU_WH,
            LEN(1) | ACU_OE | LSU_RE | dest_in,
            LEN(1) | PC_INI | chk_trap(trap),
            
            0ull, 0ull, 0ull,
        };
    }
    
    
    constexpr auto write_µcode(void) noexcept
    {
        constexpr auto NTRAP = 0x00, TRAP = 0x80;
        
        std::array<µcode_line, std::numeric_limits<std::uint8_t>::max() + 1> µcode{};
        
        // misc
        {
            µcode[NOP | NTRAP] = emit_nop(false);
            µcode[NOP |  TRAP] = emit_nop(true);
            
            µcode[BRK | NTRAP] = emit_brk();
        }
        
#define CREATE_MVB_X_Y(x, y, z) µcode[MVB_##x##_##y | NTRAP] = emit_mvb(RF_##x##I, RF_##y##O, false, z); \
                                µcode[MVB_##x##_##y |  TRAP] = emit_mvb(RF_##x##I, RF_##y##O, true,  z);
        // r0 -> other register moving
        {
            CREATE_MVB_X_Y(A, B, false)
            CREATE_MVB_X_Y(A, C, false)
            CREATE_MVB_X_Y(A, D, false)
            CREATE_MVB_X_Y(A, F, true)
        }
        
        // r1 -> other register moving
        {
            CREATE_MVB_X_Y(B, A, false)
            CREATE_MVB_X_Y(B, C, false)
            CREATE_MVB_X_Y(B, D, false)
            CREATE_MVB_X_Y(B, F, true)
        }
        
        // r2 -> other register moving
        {
            CREATE_MVB_X_Y(C, A, false)
            CREATE_MVB_X_Y(C, B, false)
            CREATE_MVB_X_Y(C, D, false)
            CREATE_MVB_X_Y(C, F, true)
        }
        
        // r3 -> other register moving
        {
            CREATE_MVB_X_Y(D, A, false)
            CREATE_MVB_X_Y(D, B, false)
            CREATE_MVB_X_Y(D, C, false)
            CREATE_MVB_X_Y(D, F, true)
        }
#undef CREATE_MVB_X_Y
        
        
#define CREATE_ALUR_X_Y(x, y, o, w) µcode[w##_##x##_##y | NTRAP] = emit_alu2reg(RF_##x##I, RF_##x##O, RF_##y##O, o, false); \
                                    µcode[w##_##x##_##y |  TRAP] = emit_alu2reg(RF_##x##I, RF_##x##O, RF_##y##O, o, true);
#define ENUM_ALUR(x, y) CREATE_ALUR_X_Y(x, y, ALU_ADD, ADC) \
                        CREATE_ALUR_X_Y(x, y, ALU_SUB, SBB) \
                        CREATE_ALUR_X_Y(x, y, ALU_AND, AND) \
                        CREATE_ALUR_X_Y(x, y, ALU_OR,  LOR)
        {
            ENUM_ALUR(A, B)
            ENUM_ALUR(A, C)
            ENUM_ALUR(A, D)
            
            ENUM_ALUR(B, A)
            ENUM_ALUR(B, C)
            ENUM_ALUR(B, D)
            
            ENUM_ALUR(C, A)
            ENUM_ALUR(C, B)
            ENUM_ALUR(C, D)
            
            ENUM_ALUR(D, A)
            ENUM_ALUR(D, B)
            ENUM_ALUR(D, C)
        }
#undef ENUM_ALUR
#undef CREATE_ALUR_X_Y

        //µcode_type dest_in, µcode_type dest_out, µcode_type op, bool trap
#define CREATE_ALUI_X(x, o, w) µcode[w##_##x##_IMM | NTRAP] = emit_aluimm(RF_##x##I, RF_##x##O, o, false); \
                               µcode[w##_##x##_IMM |  TRAP] = emit_aluimm(RF_##x##I, RF_##x##O, o, false);
#define ENUM_ALUI(x) CREATE_ALUI_X(x, ALU_ADD, ADC) \
                     CREATE_ALUI_X(x, ALU_SUB, SBB) \
                     CREATE_ALUI_X(x, ALU_AND, AND) \
                     CREATE_ALUI_X(x, ALU_OR,  LOR) \
                     CREATE_ALUI_X(x, ALU_SHL, ROL) \
                     CREATE_ALUI_X(x, ALU_SHR, ROR)
        {
            ENUM_ALUI(A)
            ENUM_ALUI(B)
            ENUM_ALUI(C)
            ENUM_ALUI(D)
        }
#undef ENUM_ALUI
#undef CREATE_ALUI

        
#define CREATE_ALU_NOT(x) µcode[NOT_##x | NTRAP] = emit_alunot(RF_##x##I, RF_##x##O, false); \
                          µcode[NOT_##x |  TRAP] = emit_alunot(RF_##x##I, RF_##x##O, true);
        {
            CREATE_ALU_NOT(A)
            CREATE_ALU_NOT(B)
            CREATE_ALU_NOT(C)
            CREATE_ALU_NOT(D)
        }
        

#define CREATE_LDB(x) µcode[LDB_##x##_IMM | NTRAP] = emit_ldbimm(RF_##x##I, false); \
                      µcode[LDB_##x##_IMM |  TRAP] = emit_ldbimm(RF_##x##I, true); \
                      µcode[LDB_##x##_MEM | NTRAP] = emit_ldbmem(RF_##x##I, false); \
                      µcode[LDB_##x##_MEM |  TRAP] = emit_ldbmem(RF_##x##I, true);
        {
            CREATE_LDB(A)
            CREATE_LDB(B)
            CREATE_LDB(C)
            CREATE_LDB(D)
        }
#undef CREATE_LDB
       
        //µcode_type src_out, bool trap
#define CREATE_STB(x) µcode[STB_MEM_##x | NTRAP] = emit_stbmem(RF_##x##O, false); \
                      µcode[STB_MEM_##x |  TRAP] = emit_stbmem(RF_##x##O, true);
        {
            CREATE_STB(A)
            CREATE_STB(B)
            CREATE_STB(C)
            CREATE_STB(D)
        }
#undef CREATE_STB
        
        
        µcode[PUSH_IP | NTRAP] = emit_puship(false);
        µcode[PUSH_IP |  TRAP] = emit_puship(true);
        
        µcode[POP_IP | NTRAP] = emit_popip(false);
        µcode[POP_IP |  TRAP] = emit_popip(true);
        
#define CREATE_STACKR(x) µcode[PUSH_##x | NTRAP] = emit_push8r(RF_##x##O, false); \
                         µcode[PUSH_##x |  TRAP] = emit_push8r(RF_##x##O, true); \
                         µcode[POP_##x | NTRAP] = emit_pop8r(RF_##x##O, false); \
                         µcode[POP_##x |  TRAP] = emit_pop8r(RF_##x##O, true);
        {
            CREATE_STACKR(A)
            CREATE_STACKR(B)
            CREATE_STACKR(C)
            CREATE_STACKR(D)
        }
#undef CREATE_STACKR
        
        µcode[JEZ_MEM | NTRAP] = emit_jump(REQUEST_JEZ, false);
        µcode[JEZ_MEM |  TRAP] = emit_jump(REQUEST_JEZ, true);
        
        µcode[JCS_MEM | NTRAP] = emit_jump(REQUEST_JCS, false);
        µcode[JCS_MEM |  TRAP] = emit_jump(REQUEST_JCS, true);
        
        µcode[JMP_MEM | NTRAP] = emit_jump(FORCE_JUMP, false);
        µcode[JMP_MEM |  TRAP] = emit_jump(FORCE_JUMP, true);
        
        
        µcode[DEREF_AB_A | NTRAP] = emit_deref(RF_AO, RF_BO, RF_AI, false);
        µcode[DEREF_AB_A |  TRAP] = emit_deref(RF_AO, RF_BO, RF_BI, true);
        
        µcode[DEREF_CD_C | NTRAP] = emit_deref(RF_CO, RF_DO, RF_CI, false);
        µcode[DEREF_CD_C |  TRAP] = emit_deref(RF_CO, RF_DO, RF_CI, true);
        
        return µcode;
    }
}




int main(int argc, const char** argv)
{
    if (argc == 2)
    {
        constexpr auto µcode = write_µcode();
        
        if (auto file = std::ofstream(argv[1]); file.good())
        {
            for (const auto& µinsn : µcode)
            {
                for (const auto& µop : µinsn)
                {
                    file.write(reinterpret_cast<char*>(µop), sizeof(µop));
                }
            }
            
            return EXIT_SUCCESS;
        }
        
        else
        {
            std::fprintf(stderr, "[Error] File %s could not be opened for writing\nExiting...\n", argv[1]);
            return EXIT_FAILURE;
        }
    }
    
    else
    {
        std::fprintf(stderr, "[Error] %i arguments were passed, but 1 was expected\nExiting...\n", (argc - 1));
        return EXIT_FAILURE;
    }
}

