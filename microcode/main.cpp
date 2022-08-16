#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <fstream>
#include <array>
#include <string>
#include <string_view>

#include <limits>


#include "opcode.h"


#define BIT(x) (1ull << x##ull)
#define IMM(x) (x << 56ull)


enum : std::uint64_t
{
    IMMEDIATE_IN = BIT(55),
    LSU_STACK_ENABLE = BIT(54),

    EAU_ADDRESS_BUS_IN = BIT(53), //not in Logisim design
    EAU_DATA_BUS_OUT = BIT(52),   //^
    
    REQUEST_JUMP = BIT(51),       //^
    
    
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
                            FETCH_INSN = FETCH_DATA | PCU_INSTRUCTION_REGISTER_IN,
                            
                            PC_INCREMENT_BOTH = PC_LOAD_VALUE | PC_SKIP_TO_NEXT_INSTRUCTION;



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
        PC_INCREMENT_BOTH,
        FETCH_DATA | ALU_WRITE_B | ALU_IN,
        
        ALU_NOT | ALU_IN | ALU_WRITE_OUT,
        ALU_OUT | RF_FLAGS_IN | dest_in,
        PC_SKIP_TO_NEXT_INSTRUCTION,
        0ull,
    };
}

µcode_line emitAluMem(µcode_type dest_in, µcode_type dest_out, µcode_type op)
{
    return
    {
        FETCH_INSN,
        dest_out | ALU_WRITE_A | ALU_IN,
        PC_INCREMENT_BOTH,
        FETCH_DATA | EAU_DATA_BUS_IN | EAU_LO_SELECT | PC_INCREMENT_BOTH, //determine endianness-should be big
        
        FETCH_DATA | EAU_DATA_BUS_IN | EAU_HI_SELECT,
        EAU_ADDRESS_BUS_OUT | LSU_RAM_ENABLE | LSU_READ_ENABLE | ALU_IN | ALU_WRITE_B,
        op | ALU_IN | ALU_WRITE_OUT,
        ALU_OUT | RF_FLAGS_IN | dest_in,
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
        PC_SKIP_TO_NEXT_INSTRUCTION,
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
        PC_SKIP_TO_NEXT_INSTRUCTION,
        0ull,
        
        0ull,
        0ull,
        0ull,
        0ull,
    };
}

µcode_line emitMvbF(µcode_type dest_in, µcode_type src_out)
{
    return
    {
        FETCH_INSN,
        dest_in | src_out | CONNECT_F_TO_DATA_BUS,
        PC_SKIP_TO_NEXT_INSTRUCTION,
        0ull,
        
        0ull,
        0ull,
        0ull,
        0ull,
    };
}

µcode_line emitLdbImm(µcode_type dest_in)
{
    return
    {
        FETCH_INSN,
        PC_INCREMENT_BOTH,
        FETCH_DATA | dest_in,
        PC_SKIP_TO_NEXT_INSTRUCTION,
        
        0ull,
        0ull,
        0ull,
        0ull,
    };
}

µcode_line emitLdbMem(µcode_type dest_in)
{
    return
    {
        FETCH_INSN,
        PC_INCREMENT_BOTH,
        FETCH_DATA | EAU_DATA_BUS_IN | EAU_LO_SELECT,
        PC_INCREMENT_BOTH,
        
        FETCH_DATA | EAU_DATA_BUS_IN | EAU_HI_SELECT,
        EAU_ADDRESS_BUS_OUT | LSU_ROM_ENABLE | LSU_READ_ENABLE | dest_in,
        PC_SKIP_TO_NEXT_INSTRUCTION,
        0ull,
    };
}

µcode_line emitStbMem(µcode_type src_out)
{
    return
    {
        FETCH_INSN,
        PC_INCREMENT_BOTH,
        FETCH_DATA | EAU_DATA_BUS_IN | EAU_LO_SELECT,
        PC_INCREMENT_BOTH,
        
        FETCH_DATA | EAU_DATA_BUS_IN | EAU_HI_SELECT,
        EAU_ADDRESS_BUS_OUT | LSU_RAM_ENABLE | LSU_WRITE_ENABLE | src_out,
        PC_SKIP_TO_NEXT_INSTRUCTION,
        0ull,
    };
}

µcode_line emitStbMemImm(void)
{
    auto temp = emitStbMem(0ull);
    
    return
    {
        temp[0],
        temp[1],
        temp[2],
        temp[3],
        
        temp[4],
        PC_INCREMENT_BOTH,
        FETCH_DATA | RF_TEMP_IN,
        temp[5] | RF_TEMP_OUT,
    };
}


µcode_line emitDeref(µcode_type src1_out, µcode_type src2_out, µcode_type dest_in)
{
    return
    {
        FETCH_INSN,
        src1_out | EAU_DATA_BUS_IN | EAU_LO_SELECT,
        src2_out | EAU_DATA_BUS_IN | EAU_HI_SELECT,
        EAU_ADDRESS_BUS_OUT | LSU_RAM_ENABLE | LSU_READ_ENABLE | dest_in,
        
        PC_SKIP_TO_NEXT_INSTRUCTION,
        0ull,
        0ull,
        0ull,
    };
}

µcode_line emitPop8(µcode_type dest_in)
{
    return
    {
        FETCH_INSN,
        LSU_STACK_ENABLE | LSU_RAM_ENABLE | LSU_READ_ENABLE | dest_in,
        LSU_DECREMENT_STACK_POINTER,
        PC_SKIP_TO_NEXT_INSTRUCTION,
        
        0ull,
        0ull,
        0ull,
        0ull,
    };
}

µcode_line emitPopIp(void)
{
    return
    {
        FETCH_INSN,
        LSU_STACK_ENABLE | LSU_RAM_ENABLE | LSU_READ_ENABLE | EAU_DATA_BUS_IN | EAU_LO_SELECT,
        LSU_DECREMENT_STACK_POINTER,
        LSU_STACK_ENABLE | LSU_RAM_ENABLE | LSU_READ_ENABLE | EAU_DATA_BUS_IN | EAU_HI_SELECT,
        
        LSU_DECREMENT_STACK_POINTER,
        EAU_ADDRESS_BUS_OUT | PC_ADDRESS_BUS_IN | PC_LOAD_VALUE,
        PC_SKIP_TO_NEXT_INSTRUCTION,
        0ull,
    };
}

µcode_line emitPush8(µcode_type src_out)
{
    return
    {
        FETCH_INSN,
        LSU_INCREMENT_STACK_POINTER,
        LSU_STACK_ENABLE | LSU_RAM_ENABLE | LSU_WRITE_ENABLE | src_out,
        PC_SKIP_TO_NEXT_INSTRUCTION,
        
        0ull,
        0ull,
        0ull,
        0ull,
    };
}

µcode_line emitPushIp(void)
{
    return
    {
        FETCH_INSN,
        LSU_INCREMENT_STACK_POINTER,
        PC_ADDRESS_BUS_OUT | EAU_ADDRESS_BUS_IN,
        EAU_LO_SELECT | EAU_DATA_BUS_OUT | LSU_STACK_ENABLE | LSU_RAM_ENABLE | LSU_WRITE_ENABLE,
        
        LSU_INCREMENT_STACK_POINTER,
        EAU_HI_SELECT | EAU_DATA_BUS_OUT | LSU_STACK_ENABLE | LSU_RAM_ENABLE | LSU_WRITE_ENABLE,
        PC_SKIP_TO_NEXT_INSTRUCTION,
        0ull,
    };
}

µcode_line emitPushImm(void)
{
    return
    {
        FETCH_INSN,
        PC_INCREMENT_BOTH,
        LSU_INCREMENT_STACK_POINTER,
        FETCH_DATA | LSU_STACK_ENABLE | LSU_RAM_ENABLE | LSU_WRITE_ENABLE,
        
        PC_SKIP_TO_NEXT_INSTRUCTION,
        0ull,
        0ull,
        0ull,
    };
}

µcode_line emitPushMem(void)
{
    return
    {
        FETCH_INSN,
        PC_INCREMENT_BOTH,
        LSU_INCREMENT_STACK_POINTER,
        FETCH_DATA | RF_TEMP_IN,
        
        PC_INCREMENT_BOTH,
        FETCH_DATA | LSU_STACK_ENABLE | LSU_RAM_ENABLE | LSU_WRITE_ENABLE,
        LSU_INCREMENT_STACK_POINTER,
        RF_TEMP_OUT | LSU_STACK_ENABLE | LSU_RAM_ENABLE | LSU_WRITE_ENABLE,
    };
}

µcode_line emitJump(µcode_type condition)
{
    return
    {
        FETCH_INSN, //in hardware circuit should be PC load = load in | jump enable (comes from decoder)
        PC_INCREMENT_BOTH,
        FETCH_DATA | EAU_DATA_BUS_IN | EAU_LO_SELECT,
        PC_INCREMENT_BOTH,
        
        FETCH_DATA | EAU_DATA_BUS_IN | EAU_HI_SELECT,
        EAU_ADDRESS_BUS_OUT | PC_ADDRESS_BUS_IN | condition,
        PC_SKIP_TO_NEXT_INSTRUCTION,
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
                
                
#define ALU_INSN(x, z, w) µcode[z##_##x##_IMM] = emitAluMem(RF_##x##_IN, RF_##x##_OUT, ALU_##w)
#define ALL_ALU_INSN(x) { ALU_INSN(x, ADC, ADDITION); ALU_INSN(x, SBB, SUBTRACTION); ALU_INSN(x, AND, AND); ALU_INSN(x, LOR, OR); }
                ALL_ALU_INSN(A);
                ALL_ALU_INSN(B);
                ALL_ALU_INSN(C);
                ALL_ALU_INSN(D);
#undef ALL_ALU_INSN
#undef ALU_INSN
                
                
#define MVB_INSN(x, y, z, w) µcode[MVB_##x##_##z] = emitMvbF(RF_##y##_IN, RF_##w##_OUT)
                MVB_INSN(A, A, F, FLAGS);
                MVB_INSN(B, B, F, FLAGS);
                MVB_INSN(C, C, F, FLAGS);
                MVB_INSN(D, D, F, FLAGS);
                
                MVB_INSN(F, FLAGS, A, A);
                MVB_INSN(F, FLAGS, B, B);
                MVB_INSN(F, FLAGS, C, C);
                MVB_INSN(F, FLAGS, D, D);
#undef MVB_INSN
                
#define LDB_INSN(x) µcode[LDB_##x##_IMM] = emitLdbImm(RF_##x##_IN)
                LDB_INSN(A);
                LDB_INSN(B);
                LDB_INSN(C);
                LDB_INSN(D);
#undef LDB_INSN
                
#define LDB_INSN(x) µcode[LDB_##x##_MEM] = emitLdbMem(RF_##x##_IN)
                LDB_INSN(A);
                LDB_INSN(B);
                LDB_INSN(C);
                LDB_INSN(D);
#undef LDB_INSN
                
#define STB_INSN(x) µcode[STB_MEM_##x] = emitStbMem(RF_##x##_OUT)
                STB_INSN(A);
                STB_INSN(B);
                STB_INSN(C);
                STB_INSN(D);
#undef STB_INSN
                
                µcode[STB_MEM_IMM] = emitStbMemImm();
                
                µcode[DEREF_AB_A] = emitDeref(RF_A_OUT, RF_B_OUT, RF_A_IN);
                µcode[DEREF_CD_C] = emitDeref(RF_C_OUT, RF_D_OUT, RF_C_IN);
                
#define POP_INSN(x, y) µcode[POP_##x] = emitPop8(y)
                POP_INSN(A, RF_A_IN);
                POP_INSN(B, RF_B_IN);
                POP_INSN(C, RF_C_IN);
                POP_INSN(D, RF_D_IN);
                POP_INSN(F, RF_FLAGS_IN  | CONNECT_F_TO_DATA_BUS);
                POP_INSN(DISCARD, 0ull);
#undef POP_INSN
               
                µcode[POP_IP] = emitPopIp();
                µcode[PUSH_IP] = emitPushIp();
                
#define PUSH_INSN(x, y) µcode[PUSH_##x] = emitPush8(y)
                PUSH_INSN(A, RF_A_OUT);
                PUSH_INSN(B, RF_B_OUT);
                PUSH_INSN(C, RF_C_OUT);
                PUSH_INSN(D, RF_D_OUT);
                PUSH_INSN(F, RF_FLAGS_OUT | CONNECT_F_TO_DATA_BUS);
#undef PUSH_INSN
                
                µcode[PUSH_IMM] = emitPushImm();
                µcode[PUSH_MEM] = emitPushMem();
                
#define JUMP_INSN(x, y) µcode[x##_MEM] = emitJump(y)
                JUMP_INSN(JGZ, JUMP_GREATER_THAN_ZERO);
                JUMP_INSN(JEZ, JUMP_ON_ZERO);
                JUMP_INSN(JCS, JUMP_CARRY_SET);
#undef JUMP_INSN
                                    
                
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
                std::fprintf(stderr, "[Error] Could not open file %s for writing\n", path.data());
                return EXIT_FAILURE;
            }
        } break;
            
        default:
            std::fprintf(stderr, "[Error] 0ull,Invalid number of arguments specified\n");
            return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
