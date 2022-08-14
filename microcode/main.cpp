#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <fstream>
#include <array>
#include <string>
#include <string_view>

#include <limits>


#include "opcode.h"

/*
 PC:
 -Address bus output enable
 -Address bus input enable
 -Increment next instruction
 -Enable
 -Load
 
 
 ALU:
 -Input enable
 -Output enable
 -Write output
 -Write alu A
 -Write alu B
 -Write alu F
 
 LSU:
 -Increment stack pointer
 -Decrement stack pointer
 -Read only memory enable
 -Random access memory enable
 -Read enable
 -Write enable
 -Flags output enable
 
 EAU:
 -Effective address high write
 -Effective address low write
 -Data bus input enable
 -Address bus output enable
 
 RF:
 -F register output enable
 -D register output enable
 -C register output enable
 -B register output enable
 -A register output enable
 -F register input enable
 -D register input enable
 -C register input enable
 -B register input enable
 -A register input enable
 
 
 
 */

#define BIT(x) (1ull << x##ull)
#define IMM(x) (x << 56ull)


enum : std::uint64_t
{
    IMMEDIATE_IN = BIT(55),
    

    
    JUMP_GREATER_THAN_ZERO = BIT(45),
    JUMP_CARRY_SET         = BIT(44),
    JUMP_ON_ZERO           = BIT(43),
    
    PCU_INSTRUCTION_REGISTER_IN = BIT(42),
    
    RF_TEMP_OUT = BIT(41),
    RF_TEMP_IN  = BIT(40),
    
    CONNECT_F_TO_DATA_BUS = BIT(39),
    
    PC_LOAD_VALUE               = BIT(38),
    PC_ENABLE                   = BIT(37), //should be unused
    PC_SKIP_TO_NEXT_INSTRUCTION = BIT(36),
    PC_ADDRESS_BUS_OUT          = BIT(35),
    PC_ADDRESS_BUS_IN           = BIT(34),

    EAU_ADDRESS_BUS_OUT = BIT(33),
    EAU_DATA_BUS_IN     = BIT(32),
    EAU_LO_SELECT       = BIT(31),
    EAU_HI_SELECT       = BIT(30),
    
    LSU_FLAGS_OUT               = BIT(29),
    LSU_WRITE_ENABLE            = BIT(28),
    LSU_READ_ENABLE             = BIT(27),
    LSU_RAM_ENABLE              = BIT(26),
    LSU_ROM_ENABLE              = BIT(25),
    LSU_DECREMENT_STACK_POINTER = BIT(24),
    LSU_INCREMENT_STACK_POINTER = BIT(23),
    
    ALU_ROTATE_RIGHT = BIT(22),
    ALU_ROTATE_LEFT  = BIT(21),
    ALU_NOT          = BIT(20),
    ALU_OR           = BIT(19),
    ALU_AND          = BIT(18),
    ALU_SUBTRACTION  = BIT(17),
    ALU_ADDITION     = BIT(16),
    
    ALU_WRITE_A   = BIT(15),
    ALU_WRITE_B   = BIT(14),
    ALU_WRITE_F   = BIT(13),
    ALU_WRITE_OUT = BIT(12),
    ALU_OUT       = BIT(11),
    ALU_IN        = BIT(10),
    
    RF_FLAGS_OUT = BIT(9),
    RF_D_OUT     = BIT(8),
    RF_C_OUT     = BIT(7),
    RF_B_OUT     = BIT(6),
    RF_A_OUT     = BIT(5),
    
    RF_FLAGS_IN = BIT(4),
    RF_D_IN     = BIT(3),
    RF_C_IN     = BIT(2),
    RF_B_IN     = BIT(1),
    RF_A_IN     = BIT(0),
    
};



static constexpr const auto µcode_length = 8;

//typedef std::uint64_t µcode_type;
using µcode_type = std::uint64_t;
using µcode_line = std::array<µcode_type, µcode_length>;



static constexpr const auto FETCH_DATA = PC_ADDRESS_BUS_OUT | LSU_ROM_ENABLE | LSU_READ_ENABLE,
                            FETCH_INSN = FETCH_DATA | PCU_INSTRUCTION_REGISTER_IN;



µcode_line emitMvb(µcode_type dest_in, µcode_type src_out)
{
    return
    {
        FETCH_INSN,
        dest_in | src_out,
        PC_SKIP_TO_NEXT_INSTRUCTION,
        0ull,
        
        0ull,
        0ull,
        0ull,
        0ull,
    };
}

µcode_line emitAlu2Reg(µcode_type dest_in, µcode_type dest_out, µcode_type src_out, µcode_type op)
{
    return
    {
        FETCH_INSN,
        dest_out | ALU_WRITE_A | ALU_IN,
        src_out  | ALU_WRITE_B | ALU_IN,
        RF_FLAGS_OUT | ALU_WRITE_F | ALU_IN,
        
        op | ALU_IN | ALU_WRITE_OUT,
        ALU_OUT | RF_FLAGS_IN | dest_in,
        PC_SKIP_TO_NEXT_INSTRUCTION,
        0ull,
    };
}

µcode_line emitAluImm(µcode_type dest_in, µcode_type dest_out, µcode_type op)
{
    return
    {
        FETCH_INSN,
        dest_out | ALU_WRITE_A | ALU_IN,
        PC_ADDRESS_BUS_OUT | PC_ADDRESS_BUS_IN | PC_LOAD_VALUE,
        FETCH_DATA | ALU_WRITE_B | ALU_IN,
        
        ALU_NOT | ALU_IN | ALU_WRITE_OUT,
        ALU_OUT | RF_FLAGS_IN | dest_in,
        PC_SKIP_TO_NEXT_INSTRUCTION,
        0ull,
    };
}

µcode_line emitAluNot(µcode_type dest_in, µcode_type dest_out)
{
    return
    {
        FETCH_INSN,
        dest_out | ALU_WRITE_A | ALU_IN,
        RF_FLAGS_OUT | ALU_WRITE_F | ALU_IN,
        ALU_NOT | ALU_IN | ALU_WRITE_OUT,
        
        ALU_OUT | RF_FLAGS_IN | dest_in,
        PC_SKIP_TO_NEXT_INSTRUCTION,
        0ull,
        0ull,
    };
}


µcode_line emitNop(void)
{
    return
    {
        FETCH_INSN,
        0ull,
        0ull,
        0ull,
        
        0ull,
        0ull,
        0ull,
        0ull,
    };
}

µcode_line emitBrk(void)
{
    return
    {
        FETCH_INSN,
        IMMEDIATE_IN | IMM(0x08ull) | CONNECT_F_TO_DATA_BUS | RF_FLAGS_IN,
        0ull,
        0ull,
        
        0ull,
        0ull,
        0ull,
        0ull,
    };
}


int main(int argc, const char** argv)
{
    switch (argc)
    {
        case 2:
        {
            std::string_view path = argv[1];
            
            std::ofstream file(path, std::ios::out | std::ios::binary);
            
            if (file.good())
            {
                std::array<std::array<std::uint64_t, 8>, std::numeric_limits<std::uint8_t>::max() + 1> µcode{ 0 };
                
                
                µcode[NOP] = emitNop();
                µcode[BRK] = emitBrk();
                
                
#define MVB_INSN(x, y) µcode[MVB_##x##_##y] = emitMvb(RF_##x##_IN, RF_##y##_OUT)
                MVB_INSN(A, B);
                MVB_INSN(A, C);
                MVB_INSN(A, D);
                
                MVB_INSN(B, A);
                MVB_INSN(B, C);
                MVB_INSN(B, D);
                
                MVB_INSN(C, A);
                MVB_INSN(C, B);
                MVB_INSN(C, D);
                
                MVB_INSN(D, A);
                MVB_INSN(D, B);
                MVB_INSN(D, C);
#undef MVB_INSN
                
                
#define ALU_INSN(x, y, z, w) µcode[z##_##x##_##y] = emitAlu2Reg(RF_##x##_IN, RF_##x##_OUT, RF_##y##_OUT, ALU_##w)
#define ALL_ALU_INSN(x, y) { ALU_INSN(x, y, ADC, ADDITION); ALU_INSN(x, y, SBB, SUBTRACTION); ALU_INSN(x, y, AND, AND); ALU_INSN(x, y, LOR, OR); }
                ALL_ALU_INSN(A, B);
                ALL_ALU_INSN(A, C);
                ALL_ALU_INSN(A, D);
                
                ALL_ALU_INSN(B, A);
                ALL_ALU_INSN(B, C);
                ALL_ALU_INSN(B, D);
                
                ALL_ALU_INSN(C, A);
                ALL_ALU_INSN(C, B);
                ALL_ALU_INSN(C, D);
                
                ALL_ALU_INSN(D, A);
                ALL_ALU_INSN(D, B);
                ALL_ALU_INSN(D, C);
#undef ALL_ALU_INSN
#undef ALU_INSN
                
                
#define NOT_INSN(x) µcode[NOT_##x] = emitAluNot(RF_##x##_IN, RF_##x##_OUT)
                NOT_INSN(A);
                NOT_INSN(B);
                NOT_INSN(C);
                NOT_INSN(D);
#undef NOT_INSN
                
                
#define ALU_INSN(x, z, w) µcode[z##_##x##_IMM] = emitAluImm(RF_##x##_IN, RF_##x##_OUT, ALU_##w)
#define ALL_ALU_INSN(x) { ALU_INSN(x, ADC, ADDITION); ALU_INSN(x, SBB, SUBTRACTION); ALU_INSN(x, AND, AND); ALU_INSN(x, LOR, OR); ALU_INSN(x, ROR, ROTATE_RIGHT); ALU_INSN(x, ROL, ROTATE_LEFT); }
                ALL_ALU_INSN(A);
                ALL_ALU_INSN(B);
                ALL_ALU_INSN(C);
                ALL_ALU_INSN(D);
#undef ALL_ALU_INSN
#undef ALU_INSN
                
                
                
                //jnz
                //and Flags with zero bit
                //load temp reg with 255
                
    
                auto counter = 0;
                
            
                for (auto i = 0; i < µcode.size(); ++i)
                {
                    for (auto j = 0; j < µcode[0].size(); ++j)
                    {
                        auto µop = µcode[i][j];
                        
                        counter++;
                        std::fprintf(stderr, "Counter: %i\n", counter);
                        
                        std::uint32_t hi = ((µop & 0xFFFFFFFF00000000) >> 32);
                        std::uint32_t lo = ((µop & 0x00000000FFFFFFFF) >> 0);
                        
                        std::uint16_t hi_hi = ((hi & 0xFFFF0000) >> 16);
                        std::uint16_t hi_lo = ((hi & 0x0000FFFF) >> 0);
                        
                        std::uint16_t lo_hi = ((lo & 0xFFFF0000) >> 16);
                        std::uint16_t lo_lo = ((lo & 0x0000FFFF) >> 0);
                        
                        
                        

                        std::uint8_t hi_hi_hi = ((hi_hi & 0xFF00) >> 8);
                        std::uint8_t hi_hi_lo = ((hi_hi & 0x00FF) >> 0);
                        
                        std::uint8_t hi_lo_hi = ((hi_lo & 0xFF00) >> 8);
                        std::uint8_t hi_lo_lo = ((hi_lo & 0x00FF) >> 0);
                        
                        
                        std::uint8_t lo_hi_hi = ((lo_hi & 0xFF00) >> 8);
                        std::uint8_t lo_hi_lo = ((lo_hi & 0x00FF) >> 0);
                        
                        std::uint8_t lo_lo_hi = ((lo_lo & 0xFF00) >> 8);
                        std::uint8_t lo_lo_lo = ((lo_lo & 0x00FF) >> 0);
                        
                        using b = std::uint8_t;
                        
#define WRITE(x) file.write(reinterpret_cast<char*>(&x), sizeof(x))
                        
                        
                        WRITE(lo_lo_hi);
                        WRITE(lo_lo_lo);
                        
                        WRITE(lo_hi_hi);
                        WRITE(lo_hi_lo);
                        
                        WRITE(hi_lo_hi);
                        WRITE(hi_lo_lo);
                        
                        WRITE(hi_hi_hi);
                        WRITE(hi_hi_lo);
                        
                        
                        
                        
                        //file.write(reinterpret_cast<char*>(&µop), sizeof(µop));
                    }
                }
                
                //file.write(buffer.data(), buffer.size() * sizeof(*buffer.data()));
            }
            else
            {
                std::fprintf(stderr, "Could not open file %s for writing\n", path.data());
                return EXIT_FAILURE;
            }
        } break;
            
        default:
            std::fprintf(stderr, "Invalid number of arguments specified\n");
            return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
