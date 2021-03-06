//=========================================================================
// Variable bit-width RISCV Core
//=========================================================================

`ifndef RISCV_CORE_V
`define RISCV_CORE_V

`include "vc-MemReqMsg.v"
`include "vc-MemRespMsg.v"
`include "param-Ctrl.v"
`include "param-Dpath.v"



module param_riscv_Core
// #(
// 
// print '  parameter P_NBITS = '+str(p.P_NBITS) + ','
// print '  parameter C_N_OFF = '+str(p.C_N_OFF) + ','
// print '  parameter C_OFFBITS = '+str(p.C_OFFBITS)
//
// )
(
  input         clk,
  input         reset,

  // Instruction Memory Request Port

  output [`VC_MEM_REQ_MSG_SZ(32,32)-1:0] imemreq_msg,
  output                                 imemreq_val,
  input                                  imemreq_rdy,

  // Instruction Memory Response Port

  input [`VC_MEM_RESP_MSG_SZ(32)-1:0] imemresp_msg,
  input                               imemresp_val,

  // Data Memory Request Port

 output [`VC_MEM_REQ_MSG_SZ(32,32)-1:0] dmemreq_msg,
 output                                 dmemreq_val,
 input                                  dmemreq_rdy,

  // Data Memory Response Port

 input [`VC_MEM_RESP_MSG_SZ(32)-1:0] dmemresp_msg,
 input                               dmemresp_val,


  // CP0 Status Register Output to Host
  output [31:0] cp0_status
);

  localparam P_NBITS = 4;
  localparam C_N_OFF = 8;
  localparam C_OFFBITS = 3;



  wire [31:0]          imemreq_msg_addr;
  wire [31:0]          imemresp_msg_data;
  wire                 imemresp_rdy;
  wire                 dmemreq_msg_rw;
  wire  [1:0]          dmemreq_msg_len;
  wire [31:0]          dmemreq_msg_addr;
  wire [31:0]          dmemreq_msg_data;
  wire [31:0]          dmemresp_msg_data;
  wire                 new_inst;
  wire                 pc_mux_sel_Xhl;
  wire                 pc_plus4_mux_sel_Xhl;
  wire [4:0]           rega_addr_Rhl;
  wire [4:0]           regb_addr_Rhl;
  wire [4:0]           wb_addr_Xhl;
  wire                 mem_access_Xhl;
  wire                 wb_en_Xhl;
  wire                 wb_to_temp_Xhl;
  // ALU Inputs
  wire                 a_read_temp_reg_Xhl;
  // wire                 a_mux_sel_Rhl;
  wire                 a_mux_sel_Xhl;
  wire [3:0] b_imm_Xhl;

  wire                 b_mux_sel_Xhl;
  // ALU Outputs
  wire [2:0] a_subword_off_Rhl;
  wire [2:0] b_subword_off_Rhl;
  wire [2:0] wb_subword_off_Xhl;

  wire                 addsub_fn_Xhl;
  wire [1:0]           logic_fn_Xhl;
  wire [1:0]           shift_fn_Xhl;
  wire [1:0]           alu_fn_type_Xhl;

  wire                 prop_carry_Xhl;
  wire                 carry_in_1_Xhl;
  wire                 flag_reg_en_Xhl;
  wire                 shift_dir_sel_Xhl;
  wire                 addr_reg_en_Xhl;
  wire                 last_uop_Xhl;
  wire                 br_reg_en_Xhl;

  wire                 b_use_imm_reg_Xhl;
  wire  [4:0]          shamt_reg;
  wire [31:0]          proc2cop_data_Xhl;
  //----------------------------------------------------------------------
  // Pack Memory Request Messages
  //----------------------------------------------------------------------

  assign imemreq_msg = {1'b0, imemreq_msg_addr, 2'b0, 32'b0};

  assign dmemreq_msg = {dmemreq_msg_rw, dmemreq_msg_addr, dmemreq_msg_len, dmemreq_msg_data};

  //----------------------------------------------------------------------
  // Unpack Memory Response Messages
  //----------------------------------------------------------------------

  assign imemresp_msg_data = imemresp_msg[31:0];

  assign dmemresp_msg_data = dmemresp_msg[31:0];
  
  //----------------------------------------------------------------------
  // Control Unit
  //----------------------------------------------------------------------

  param_Ctrl ctrl
  (
    .clk                    (clk),
    .reset                  (reset),

    // Instruction Memory Port

    .imemreq_val            (imemreq_val),
    .imemreq_rdy            (imemreq_rdy),
    .imemresp_msg_data      (imemresp_msg_data),
    .imemresp_val           (imemresp_val),
    .imemresp_rdy           (imemresp_rdy),

    // Data Memory Port

   .dmemreq_msg_rw         (dmemreq_msg_rw),
   .dmemreq_msg_len        (dmemreq_msg_len),
   .dmemreq_val            (dmemreq_val),
   .dmemreq_rdy            (dmemreq_rdy),
   .dmemresp_val           (dmemresp_val),

    .pc_mux_sel_Xhl             (pc_mux_sel_Xhl),

    .pc_plus4_mux_sel_Xhl       (pc_plus4_mux_sel_Xhl),
    .rega_addr_Rhl              (rega_addr_Rhl),
    .regb_addr_Rhl              (regb_addr_Rhl),
    .wb_addr_Xhl                (wb_addr_Xhl),
    .mem_access_Xhl             (mem_access_Xhl),
    .wb_en_Xhl                  (wb_en_Xhl),
    // .wb_to_temp_Xhl             (wb_to_temp_Xhl),
    // .a_read_temp_reg_Rhl        (a_read_temp_reg_Rhl),
    // .b_mux_sel_Rhl              (b_mux_sel_Rhl), 
    // .a_read_temp_reg_Xhl        (a_read_temp_reg_Xhl),
    // .a_mux_sel_Rhl              (a_mux_sel_Rhl),
    .a_mux_sel_Xhl              (a_mux_sel_Xhl),
    .b_imm_Xhl                  (b_imm_Xhl),
    .b_mux_sel_Xhl              (b_mux_sel_Xhl),

    .a_subword_off_Rhl          (a_subword_off_Rhl),
    .b_subword_off_Rhl          (b_subword_off_Rhl),
    .wb_subword_off_Xhl         (wb_subword_off_Xhl),
    .addsub_fn_Xhl              (addsub_fn_Xhl),
    .logic_fn_Xhl               (logic_fn_Xhl),
    .shift_fn_Xhl               (shift_fn_Xhl),
    .alu_fn_type_Xhl            (alu_fn_type_Xhl),

    .prop_carry_Xhl             (prop_carry_Xhl),
    .carry_in_1_Xhl             (carry_in_1_Xhl),
    .flag_reg_en_Xhl            (flag_reg_en_Xhl),
    .shift_dir_sel_Xhl          (shift_dir_sel_Xhl),
    .addr_reg_en_Xhl            (addr_reg_en_Xhl),
    .last_uop_Xhl               (last_uop_Xhl),
    .br_reg_en_Xhl              (br_reg_en_Xhl),

    .b_use_imm_reg_Xhl          (b_use_imm_reg_Xhl),
    .shamt_reg                  (shamt_reg),
    .proc2cop_data_Xhl          (proc2cop_data_Xhl),

    .cp0_status             (cp0_status)

  );

  //----------------------------------------------------------------------
  // Datapath
  //----------------------------------------------------------------------

  param_Dpath
  // #(
  //   .P_NBITS(P_NBITS),
  //   .C_N_OFF(C_N_OFF),
  //   .C_OFFBITS(C_OFFBITS)
  // )
  dpath
  (
    .clk                     (clk),
    .reset                   (reset),

    // Instruction Memory Port

    .imemreq_msg_addr        (imemreq_msg_addr),

    // Data Memory Port

   .dmemreq_msg_addr        (dmemreq_msg_addr),
   .dmemreq_msg_data        (dmemreq_msg_data),
   .dmemresp_msg_data       (dmemresp_msg_data),
   .dmemresp_val_Xhl        (dmemresp_val),

    // Controls Signals (ctrl->dpath)
    .pc_mux_sel_Xhl             (pc_mux_sel_Xhl),
    .pc_plus4_mux_sel_Xhl       (pc_plus4_mux_sel_Xhl),
    .rega_addr_Rhl              (rega_addr_Rhl),
    .regb_addr_Rhl              (regb_addr_Rhl),
    .wb_addr_Xhl                (wb_addr_Xhl),
    .mem_access_Xhl             (mem_access_Xhl),
    .wb_en_Xhl                  (wb_en_Xhl),
    // .wb_to_temp_Xhl             (wb_to_temp_Xhl),
    // .a_read_temp_reg_Xhl        (a_read_temp_reg_Xhl),
    // .a_mux_sel_Rhl              (a_mux_sel_Rhl),
    
    .a_mux_sel_Xhl              (a_mux_sel_Xhl),
    .b_imm_Xhl                  (b_imm_Xhl),
    .b_mux_sel_Xhl              (b_mux_sel_Xhl),

    .a_subword_off_Rhl          (a_subword_off_Rhl),
    .b_subword_off_Rhl          (b_subword_off_Rhl),
    .wb_subword_off_Xhl         (wb_subword_off_Xhl),
    .addsub_fn_Xhl              (addsub_fn_Xhl),
    .logic_fn_Xhl               (logic_fn_Xhl),
    .shift_fn_Xhl               (shift_fn_Xhl),
    .alu_fn_type_Xhl            (alu_fn_type_Xhl),

    .prop_carry_Xhl             (prop_carry_Xhl),
    .carry_in_1_Xhl             (carry_in_1_Xhl),
    .flag_reg_en_Xhl            (flag_reg_en_Xhl),
    .shift_dir_sel_Xhl          (shift_dir_sel_Xhl),
    .addr_reg_en_Xhl            (addr_reg_en_Xhl),
    .last_uop_Xhl               (last_uop_Xhl),
    .br_reg_en_Xhl              (br_reg_en_Xhl),

    .b_use_imm_reg_Xhl          (b_use_imm_reg_Xhl),
    .shamt_reg                  (shamt_reg),
    .proc2cop_data_Xhl          (proc2cop_data_Xhl)

  );

endmodule

`endif

