#include <cstdint>
#include <cstdlib>

#include <format>
#include <iostream>
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
        //this is a bit hacky
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
        LSU_SP_D = BIT(12), //stack pointer direction
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
        OUT_Q1 = BIT(59),
        OUT_Q2 = BIT(60),
        SET_HALT = BIT(61),
        // /misc
    };
    
    static constexpr const auto FETCH_INSTRUCTION = PC_OE | LSU_RE | IR_WE;
    
    //determine how the instruction register will be written to after a reset to begin
    
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
    
    consteval µcode_line emit_mvb(µcode_type dest_in, µcode_type src_out, bool trap) noexcept
    {
        return
        {
            LEN(1) | FETCH_INSTRUCTION,
            LEN(1) | src_out | dest_in,
            LEN(1) | PC_INI | chk_trap(trap),
            
            0ull, 0ull, 0ull, 0ull, 0ull
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
            
            0ull, 0ull, 0ull
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
            
            0ull, 0ull, 0ull
        };
    }
    
    consteval µcode_line emit_alumem(µcode_type dest_in, µcode_type dest_out, µcode_type op, bool trap) noexcept
    {
        return
        {
            LEN(3) | FETCH_INSTRUCTION,
            LEN(3) | dest_out | ALU_WA,
            LEN(3) | OUT_Q1 | ACU_WL,
            LEN(3) | OUT_Q2 | ACU_WH,
            LEN(3) | ACU_OE | LSU_RE | ALU_WB,
            LEN(3) | op | ALU_OE | dest_in | RF_FI,
            LEN(3) | PC_INI | chk_trap(trap),
            
            0ull
        };
    }
    
}


int main(int argc, const char** argv)
{
    if (argc == 2)
    {
        
    }
    
    else
    {
        std::cout << std::format("[Error] {} arguments were passed, but 1 was expected", (argc - 1)) << std::endl;
    }
}

