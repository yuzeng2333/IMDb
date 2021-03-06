#include "rbm.hpp"

u8 round_(DATA05_D num)//round: compare the first bit of mentissa
{
   DATA05_D tmp = 0;
   tmp.assign_bit(31, num[31]);
   tmp.assign_bit(30, num[30]);
   tmp.assign_bit(29, num[29]);

   if(num[28])
      return (tmp.to_uint() + 1);
   else
      return (tmp.to_uint());

}

DATA01_D sigmoid(DATA_sum_ sum)//sigmoid using Piecewise linear approximation of nonlinearities(PLAN)
{
   DATA01_D prob, tmp1;
   if(sum >= 5)
      prob = 1.0;
   else if ( sum <= -5)
      prob = 0.0;
   else if ( sum < 5 && sum >= (fconst) 2.375)
   {
      tmp1 = (DATA01_D)0.03125 * sum;
      prob = tmp1 + (DATA01_D)0.84375;
   }
   else if ( sum > -5 && sum <= (fconst) - 2.375)
   {
      tmp1 = (DATA01_D)0.03125 * sum;
      prob = (DATA01_D)0.15625 + tmp1;
   }
   else if (sum < (fconst) 2.375 && sum >= 1)
   {
      tmp1 = (DATA01_D)0.125 * sum;
      prob = tmp1 + (DATA01_D)0.625;
   }
   else if (sum > (fconst) - 2.375 && sum <= -1)
   {
      tmp1 = (DATA01_D)0.125 * sum;
      prob = (DATA01_D)0.375 + tmp1;
   }
   else
   {
      tmp1 = (DATA01_D)0.25 * sum;
      prob = tmp1 + (DATA01_D)0.5;
   }
   return prob;
}

DATA01_D rbm::rand_gen()/*return random number between 0-1*/
{
   u32 y, tmpy1, tmpy2;
   int32 mti;
   mti = mti_signal.read();
   u32 tmp1, tmp2, tmp3, tmp4, tmp5, tmp6;

   if (mti >= N) /* generate N words at one time */
   {
      int32 kk, tmpk, tmpk1;
      if (mti == N + 1) /* initialization of the array*/
      {
         mt[0] = 5489UL & 0xffffffffUL;
         wait();
      RAND_LOOP1:
         for (mti = 1; mti < N; mti++)
         {
            tmp6 = mti - 1;
            tmp1 = mt[tmp6];
            wait();
            tmp3 = tmp1 >> 30;
            tmp4 = tmp1 ^ tmp3;
            tmp5 = 1812433253UL * tmp4;
            tmp2 = tmp5 + mti;
            /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. *//* In the previous versions, MSBs of the seed affect   */
            /* only MSBs of the array mt[].                        */
            /* 2002/01/09 modified by Makoto Matsumoto             */
            mt[mti] = tmp2 & 0xffffffffUL;
            wait();
         }

      }

   RAND_LOOP2:
      for (kk = 0; kk < N - M; kk++)
      {
         tmp1 = mt[kk];
         tmpk = kk + 1;
         wait();
         tmp2 = mt[tmpk];
         tmpk1 = kk + M;
         tmp4 = tmp1 & UPPER_MASK;
         wait();
         tmp3 = mt[tmpk1];
         tmp5 = tmp2 & LOWER_MASK;
         y = tmp4 | tmp5;
         wait();

         tmp2 = y >> 1;
         tmp1 = tmp3 ^ tmp2;
         if(y & 0x1UL)
            mt[kk] = tmp1 ^ mag1;
         else
            mt[kk] = tmp1 ^ mag0;
         wait();

      }

   RAND_LOOP3:
      for (kk = N - M; kk < N - 1; kk++)
      {
         tmp1 = mt[kk];
         tmpk = kk + 1;
         wait();
         tmp2 = mt[tmpk];
         tmpk1 = kk + (M - N);
         tmp4 = tmp1 & UPPER_MASK;
         wait();
         tmp3 = mt[tmpk1];
         tmp5 = tmp2 & LOWER_MASK;
         y = tmp4 | tmp5;
         wait();
         tmp2 = y >> 1;
         tmp1 = tmp3 ^ tmp2;
         if(y & 0x1UL)
            mt[kk] = tmp1 ^ mag1;
         else
            mt[kk] = tmp1 ^ mag0;
         wait();
      }
      tmp1 = mt[N - 1];
      wait();
      tmp2 = mt[0];
      wait();
      tmp3 = mt[M - 1];
      wait();
      tmp4 = tmp1 & UPPER_MASK;
      tmp5 = tmp2 & LOWER_MASK;
      y = tmp4 | tmp5;
      tmp2 = y >> 1;
      tmp1 = tmp3 ^ tmp2;
      if(y & 0x1UL)
         mt[kk] = tmp1 ^ mag1;
      else
         mt[kk] = tmp1 ^ mag0;
      wait();
      mti = 0;
   }
   y = mt[mti];

   wait();
   mti++;
   mti_signal.write(mti);
   /* Tempering */
   tmpy1 = y >> 11;
   y ^= tmpy1;
   tmpy1 = y << 7;
   tmpy2 = tmpy1 & 0x9d2c5680UL;
   y ^= tmpy2;
   tmpy1 = y << 15;
   tmpy2 = tmpy1 & 0xefc60000UL;
   y ^= tmpy2;
   tmpy1 = y >> 18;
   y ^= tmpy1;

   DATA01_D result = y * DATA01_D(FRACTION);

   return (result);

}

void rbm::activateHiddenUnits_train(bool pingpong, u16 nh, u16 nv)//train_flag call random; visible is the real grading
{
   // Calculate activation energy for hidden units
   u16 h, v;   // can be u16
   DATA_sum_ sum, tmp;
   DATA01_D prob;
   DATA01_D RAND;
   u8 current_data;
   DATA01_D tmp1;// Activate hidden units

ACTIVATE_HIDDEN_TRAIN_H:
   for (h = 0; h < NUM_HIDDEN_MAX; h++) //[8]
   {
      if (h == nh)
         break;
      // Get the sum of energies  //a_i=???_jw_ijx_j
      sum = 0;//TODO: hiddenenergies maximum 300 k.1 added together, k>0
   ACTIVATE_HIDDEN_TRAIN_V:
      for (v = 0; v < NUM_VISIBLE_MAX + 1; v++) // remove the +1 if you want to skip the bias (BIAS?) //[65]
      {
         if (v == nv + 1)
            break;
         current_data = data[pingpong][v];

         wait(); // [90]
         if(current_data == 1) //[91]
         {
            tmp = edges[v][h];
            sum += tmp;
            wait(); //[92]
         }
      }
      prob = sigmoid(sum);

      RAND = rand_gen();

      bool comp = (RAND < prob);
      bool th;
      if (comp)
         th = 1;
      else
         th = 0;
      hidden_unit[h] = th;
      wait();	//[89]
   }

   hidden_unit[nh] = 1; // turn on bias
   wait();  // [9]
}

void rbm::activateHiddenUnits_predict(bool pingpong, u16 nh, u16 nv)
{
   u16 h, v;
   DATA_sum_ sum, tmp;
   DATA01_D prob, tmp1;
ACTIVATE_HIDDEN_PREDICT_H:
   for (h = 0; h < NUM_HIDDEN_MAX; h++) // [6]
   {
      if (h == nh)
         break;
      sum = 0;

   ACTIVATE_HIDDEN_PREDICT_V:
      for (v = 0; v < NUM_VISIBLE_MAX + 1; v++) // remove the +1 if you want to skip the bias (BIAS?) // [33][34]
      {
         if (v == nv + 1)
            break;
         u8 current_data = data[pingpong][v];
         wait();  // [36]
         if(current_data == 1) // [37]
         {
            tmp = edges[v][h];
            wait(); // [38]
            sum += tmp;
         }
      }

      prob = sigmoid(sum);

      bool comp = ((DATA01_D)0.5 < prob);
      bool th;
      if (comp)
         th = 1;
      else
         th = 0;
      hidden_unit[h] = th;
      wait();  // [35]
   }

   hidden_unit[nh] = 1; // turn on bias
   wait(); // [7]
}


void rbm::activateVisibleUnits_train(u16 nh, u16 nv)
{
   // Calculate activation energy for visible units
   u16 v, h; //u16;
   DATA_sum_ sum;
   DATA_sum_ max;
   DATA01_D probs;
   DATA01_D RAND;
   DATA_pow pow2[K];
   DATA_sum3 sumOfpow2;
   s16 tmp;
   DATA_sum_ tmp1;
   bool tmp2;

ACTIVATE_VISIBLE_TRAIN_V:
   for (v = 0; v < NUM_VISIBLE_MAX; v += K) // [10]
   {
      if (v == nv)
         break;
      // Get the sum of energies
      u16 j;//u16
      max = -500;
   ACTIVATE_VISIBLE_TRAIN_ENERGY:
      for (j = 0; j < K; j++) // [20]
      {
         sum = 0;
      ACTIVATE_VISIBLE_TRAIN_H:
         for (h = 0; h < NUM_HIDDEN_MAX + 1; h++) // [21]
         {
            if (h == nh + 1)
               break;
            tmp2 = hidden_unit[h];
            wait();	// [56] [57]
            tmp1 = (DATA_sum_)edges[v + j][h];
            wait(); // [58]
            if(tmp2)
            {
               sum += tmp1;
            }
            wait();	// [59]   -> [22] or [56]
         }
         visibleEnergies[j] = sum;
         wait();	// [22] -> [21] [23]
         if ( sum > max )
            max = sum;
      }

      max -= 31;
   ACTIVATE_VISIBLE_TRAIN_ENERGY_UPDATE:
      for (j = 0; j < K; j++) // [23]
      {
         tmp1 = visibleEnergies[j];
         wait();	// [24]
         DATA_sum_ ve_current = tmp1 - max;
         wait();	// [25]
         visibleEnergies[j] = ve_current;
         wait();	// [26]  -> [24] or [27]
      }
      sumOfpow2 = 0; // this is the numerator

   ACTIVATE_VISIBLE_TRAIN_SUM:
      for (j = 0; j < K; j++) // [27]  [29]->[28]/[30]
      {
         /*TODO:fix point; exp function*/
         tmp1 = visibleEnergies[j];
         wait();	// [28]
         if (tmp1[0] == 0)
         {
            tmp = tmp1.to_int();
         }
         else if (tmp1 > 0)
         {
            tmp = tmp1.to_int() + 1;
         }
         else
            tmp = -1;

         DATA_pow dp;
         if (tmp < 0 )
            dp = 0;
         else
            dp = 1UL << tmp;

         sumOfpow2 += dp;
         pow2[j] = dp;
      }

      // Getting the probabilities
   ACTIVATE_VISIBLE_TRAIN_PROB:
      for (j = 0; j < K; j++) // [31]
      {
         RAND = rand_gen();
         DATA_pow dp = pow2[j];
         wait();
         probs = sld::udiv_func<32, 32, 64, 64, 64, 1>(dp, sumOfpow2);
         bool tmpb = 0;
         if (RAND < probs)
            tmpb = 1;
         visible_unit[v + j] = tmpb;
         wait();
      }
   }
   visible_unit[nv] = 1; // turn on bias
   wait(); // [11]
}

void rbm::activateVisibleUnits_predict(u16 nh, u16 nv)
{
   // Calculate activation energy for visible units
   u16 v, h; //u16;
   DATA_sum_ sum;
   DATA_sum_ max;
   DATA01_D probs;
   DATA_pow pow2[K];
   DATA_sum3 sumOfpow2;
   DATA05_D expectation;
   u8 prediction;
   s16 tmp;
   DATA_sum_ tmp1;
   bool tmp2;
ACTIVATE_VISIBLE_PREDICT_V:
   for (v = 0; v < NUM_VISIBLE_MAX; v += K)
   {
      if (v == nv)
         break;
      // Get the sum of energies
      u16 j;//u16
      max = -500;
   ACTIVATE_VISIBLE_PREDICT_ENERGY:
      for (j = 0; j < K; j++) // [14]
      {
         sum = 0;//TODO: maximum 500 k.1 added together, k>0
      ACTIVATE_VISIBLE_PREDICT_H:
         for (h = 0; h < NUM_HIDDEN_MAX + 1; h++) // remove the +1 if you want to skip the bias // [15]
         {
            if (h == nh + 1)
               break;

            tmp2 = hidden_unit[h];
            wait(); // [30] (<--[29])
            tmp1 = edges[v + j][h];
            wait(); // [31]
            if(tmp2)
            {
               sum += tmp1;
            }
            wait();  // [32]
         }
         visibleEnergies[j] = sum;
         wait();  // [16]
         if ( sum > max )
            max = sum;
      }

      max -= 31;
   ACTIVATE_VISIBLE_PREDICT_ENERGY_UPDATE:
      for (j = 0; j < K; j++) // [17]
      {
         tmp1 = visibleEnergies[j];
         wait();  //[19]
         visibleEnergies[j] = tmp1 - max;
         wait();  //[20]
      }

      sumOfpow2 = 0; // this is the numerator
      //TODO: maximum exp of 300 k.1 added together, k>0
   ACTIVATE_VISIBLE_PREDICT_SUM:
      for (j = 0; j < K; j++) // [21] [23]
      {
         /*TODO:fix point; exp function*/
         tmp1 = visibleEnergies[j];
         wait(); // [22]
         if (tmp1[0] == 0)
            tmp = tmp1.to_int();
         else if (tmp1 > 0)
         {
            tmp = tmp1.to_int() + 1;
         }
         else
            tmp = -1;

         DATA_pow dp;
         if (tmp < 0 )
            dp = 0;
         else
            dp = 1UL << tmp;

         sumOfpow2 += dp;
         pow2[j] = dp;
      }

      // Getting the probabilities
      expectation = 0.0;
   ACTIVATE_VISIBLE_PREDICT_PROB:
      for (j = 0; j < K; j++) // [24]
      {
         DATA_pow dp = pow2[j];
         wait();  // [25]
         probs = sld::udiv_func<32, 32, 64, 64, 64, 1>(dp, sumOfpow2);
         expectation += j * probs;
      }

      prediction = round_(expectation);
   ACTIVATE_VISIBLE_PREDICT_RATE:
      for (j = 0; j < K; j++) // [27]
      {
         if (j == prediction)
            predict_vector[v + j] = 1;
         else
            predict_vector[v + j] = 0;
         wait(); // [28]
      }
   }

   predict_vector[nv] = 1; // turn on bias
   wait();
}

/*
bool rbm::compare(DATA01_D prob,  bool train_flag)//TODO: prob 0-1 double
{
   DATA01_D RAND;//TODO:RAND 0-1 double
   bool hk;
   if (train_flag==1)
   {RAND=rand_gen();//get random from testbench
   //cout<<"RAND"<<RAND<<endl;
   }
   else
   RAND=0.5;
   // //cout<<RAND<<endl;
   if (RAND < prob)
      hk = 1;
   else
      hk = 0;
   return hk;
}
*/
void rbm::config()
{
RESET_CONFIG:  // Initialization
   num_hidden.write(0);
   num_visible.write(0);
   num_users.write(0);
   num_loops.write(0);
   num_movies.write(0);
   init_done.write(false);

   wait();

CONFIG_REGISTER_WHILE:
   do { wait(); }
   while (!conf_done.read());

   ///read configuration parameters
   num_hidden.write(conf_num_hidden.read());
   num_visible.write(conf_num_visible.read());
   num_users.write(conf_num_users.read());
   num_loops.write(conf_num_loops.read());
   num_testusers.write(conf_num_testusers.read());
   num_movies.write(conf_num_movies.read());

   std::cout << "Config done" << std::endl;
   ///configuration done; start other processes
   init_done.write(true);
CONFIG_IDLE_WHILE:
   while (true)
   {
      wait();
   }
}

void rbm::load()
{
RESET_LOAD:
   data_in.reset_get();
   rd_index.write(0);
   rd_length.write(0);
   rd_request.write(false);

   train_input_done.write(false);
   predict_input_done.write(false);

LOAD_INITIAL_WHILE:
   do {wait();}
   while(!init_done.read());

   const u32 nv = num_visible.read();
   const u32 ntu = num_testusers.read();
   const u32 nu = num_users.read();
   const u32 nlp = num_loops.read();
   const u32 nm = num_movies.read();
   // std::cout<<"conf result: nv-ntu-nu-nlp"<<nv<<" "<<ntu<<" "<<nu<<" "<<nlp<<std::endl;
   u16 index = 0;
   u16 loop_count = 0;
   bool pingpong = 0;
   u16 tmp1, tmp2;
LOAD_FIRST_INPUT:
   // Send DMA request
   rd_index.write(index);
   rd_length.write(5 * nm);
   //std::cout<<"DUT load index"<<index <<" loop #"<<loop_count<<std::endl;
   // 4-phase handshake
   if ( loop_count == nlp )
      do { wait(); }
      while (!train_done.read());

   //wait for training done
   rd_request.write(true);
   do { wait(); }
   while(!rd_grant.read());
   rd_request.write(false);

   tmp1 = loop_count * nu;
   tmp2 = tmp1 + index;
   pingpong = tmp2 % 2;
   // cout<<"loading start user"<<index<<":"<<pingpong<<endl;

LOAD_FIRST_USER_DATA:
   for (u16 i = 0; i < NUM_VISIBLE_MAX; i++)
   {
      if (i == nv)
         break;
      u8 rate = (u8) data_in.get();
      wait();
      data[pingpong][i] = rate;
      //cout<<data[i];
   }
   wait();
   data[pingpong][nv] = 1;

LOAD_INPUT_WHILE:
   while(true)
   {
      if ( loop_count != nlp ) //loading training data done
      {
         // 4-phase handshake with train
         train_input_done.write(true);
         do { wait(); }
         while (!train_start.read());
         train_input_done.write(false);
         do { wait(); }
         while (train_start.read());
      }
      else//loading test data done
      {
         // 4-phase handshake with predict
         predict_input_done.write(true);
         do { wait(); }
         while (!predict_start.read());
         predict_input_done.write(false);
         do { wait(); }
         while (predict_start.read());
         //cout<<"ka"<<endl;
      }

      //upgrate index, check if train loop #
      index += 1;
      if( (index == nu) &&  (loop_count != nlp))
      {
         loop_count += 1;
         index = 0;
      }
      if( (index == ntu) && (loop_count == nlp))
         // Input complete; wait for reset
      LOAD_DONE:
         do { wait(); }
         while (true);
	  /*##@@##  This is the end of a micro-inst */
		
      // Send DMA request
      unsigned dma_index = nv * (unsigned) index ;
      if (loop_count == nlp)
         dma_index += nv * nu;
      rd_index.write(dma_index);
      rd_length.write(5 * nm);
      //std::cout<<"DUT load index"<<index <<" loop #"<<loop_count<<std::endl;
      // 4-phase handshake
      if ( loop_count == nlp )
         do { wait(); }
         while (!train_done.read());
      //wait for training done
      rd_request.write(true);
      do { wait(); }
      while(!rd_grant.read());
      rd_request.write(false);

      tmp1 = loop_count * nu;
      tmp2 = tmp1 + index;
      pingpong = tmp2 % 2;
      // cout<<"loading start user"<<index<<":"<<pingpong<<endl;
	  
   LOAD_ONE_USER_DATA:
      for (u16 i = 0; i < NUM_VISIBLE_MAX; i++)
      {
         if (i == nv)
            break;
         u8 rate = data_in.get();
         wait();
         data[pingpong][i] = rate;
         //cout<<data[i];
      }
      wait();
      data[pingpong][nv] = 1;


   }
}


void rbm::train_rbm()
{
   //reset
   train_start.write(false);
   train_done.write(false);
   mti_signal.write(625);

   // Wait for configuratin
   do { wait(); }  // [0]
   while (!init_done.read());

   const u32 nv = num_visible.read();
   const u32 nh = num_hidden.read();
   const u32 nu = num_users.read();
   const u32 nlp = num_loops.read();

   u32 user;
   u32 current_loop = 0;

   u32 v;
   u32 h;
   u32 tmp3, tmp4;

   u8 tmp5;
   bool pingpong = 0;
   bool tmp1, tmp2, bool1, bool2;
   DATA_edge tmp;

   ///reset edges
RESET_LOOP_V:
   for (v = 0; v < NUM_VISIBLE_MAX + 1; v++) //[1]
   {
      if(v == nv + 1)
         break;
   RESET_LOOP_H:
      for (h = 0; h < NUM_HIDDEN_MAX + 1; h++)
      {
         if ( h == nh + 1)
            break;
         edges[v][h] = (fconst)0;
      }
   }

   u32 current_h = 0;
   u32 current_v = 0;
   u32 vlp = nv / NUM_VISIBLE_MAX ;
   u32 hlp  = nh / NUM_HIDDEN_MAX ;
   u32 vrest_ = nv - vlp * NUM_VISIBLE_MAX ;
   u32 hrest_ = nh - hlp * NUM_HIDDEN_MAX ;
   if (vrest_ != 0)
      vlp += 1;
   if (hrest_ != 0)
      hlp += 1;
   std::cout << "Configuration: VISIBLE_LOOPS- " << vlp << "; HIDDEN_LOOPS-" << hlp << std::endl;

   ///start training loop
TRAIN_LOOP:
   while (true) //[3]
   {
      if (current_loop == nlp)
      {
         // training done
         train_done.write(true);
      TRAIN_IDLE:
         do { wait(); }
         while(true);
      }
      cout << "Current training loop-" << current_loop << endl;
   TRAIN_USER:
      for (user = 0; user < nu; user++) // [5]
      {
         // ==> Phase 1: Activate hidden units

         // 4-phase handshake with load
         do { wait(); } // [6]
         while (!train_input_done.read());
         train_start.write(true);
         do { wait(); }  // [7]
         while (train_input_done.read());
         train_start.write(false);

         tmp3 = current_loop * nu;
         tmp4 = tmp3 + user;
         pingpong = tmp4 % 2;

         // Activate hidden units
         activateHiddenUnits_train(pingpong, nh, nv); //compute its activation energy a_i

         // Get positive association
      TRAIN_UPDATE_POS_V:
         for (v = 0; v < NUM_VISIBLE_MAX + 1; v++)
         {
            if(v == nv + 1)
               break;
            tmp5 = data[pingpong][v];
            wait();
            if (tmp5 != 2)
            {
            TRAIN_UPDATE_POS_H:
               for (h = 0; h < NUM_HIDDEN_MAX + 1; h++)
               {
                  if ( h == nh + 1)
                     break;
                  tmp2 = hidden_unit[h];
                  wait();
                  pos[v][h] = tmp5 && tmp2;
                  wait();
               }
            }
         }
         // ==> Phase 2: Reconstruction (activate visible units)
         // Activate visible units
         activateVisibleUnits_train(nh, nv); //compute its activation energy a_i

         // Get negative association
      TRAIN_UPDATE_NEG_V:
         for (v = 0; v < NUM_VISIBLE_MAX + 1; v++)
         {
            if (v == nv + 1)
               break;
            tmp5 = data[pingpong][v];
            wait();
         TRAIN_UPDATE_NEG_H:
            for (h = 0; h < NUM_HIDDEN_MAX + 1; h++)
            {
               if ( h == nh + 1)
                  break;
               if (tmp5 != 2)
               {
                  tmp1 = hidden_unit[h];
                  tmp2 = visible_unit[v];
                  wait();
                  neg[v][h] = tmp1 && tmp2;
                  wait();
               }
               tmp = edges[v][h];
               bool1 = pos[v][h];
               bool2 = neg[v][h];
               wait();
               // ==> Phase 3: Update the weights
               DATA_edge de;
               if(bool1 && !bool2)
               {
                  de = tmp + DATA_edge(LEARN_RATE);
               }
               else if(!bool1 && bool2)
               {
                  de = tmp - DATA_edge(LEARN_RATE);
               }
               else
               {
                  de = tmp;
               }

               edges[v][h] = de;
               wait();
            }
         }
      }
      // start next loop
      current_loop += 1;
   }
}

void rbm::predict_rbm()
{
   // Reset
   predict_start.write(false);
   predict_done.write(false);

   // Wait for configuratin
   do { wait(); }
   while (!init_done.read());

   const u16 ntu = num_testusers.read();
   const u16 nv = num_visible.read();
   const u16 nh = num_hidden.read();
   const u16 nu = num_users.read();
   const u16 nlp = num_loops.read();

   u16 user = 0;
   u8 prediction, j;
   u8 count = 0;
   u16 i, tmp1, tmp2;
   bool pingpong;

PREDICT_LOOP:
   while (true)
   {

      if (user == ntu) //prediction done
      {
         // Wait for reset
         do { wait(); }
         while(true);

      }


      // 4-phase handshake
      do { wait(); }
      while (!predict_input_done.read());
      predict_start.write(true);
      do { wait(); }
      while (predict_input_done.read());
      predict_start.write(false);

      do { wait(); }
      while (!output_done.read());

      tmp1 = nlp * nu;
      tmp2 = tmp1 + user;
      pingpong = (tmp2) % 2;

      activateHiddenUnits_predict(pingpong, nh, nv); //getting preference
      activateVisibleUnits_predict(nh, nv); //making rate based on existing edges value

   PREDICT_RATE:
      // Go through K visible units at a time
      for (i = 0; i < NUM_VISIBLE_MAX; i += K)
      {
         if(i == nv)
            break;
         prediction = 0;
      PREDICT_RESULT:
         for (j = 0; j < K; j++)
         {
            bool current_pv = predict_vector[i + j];
            wait();  // [13]
            if (current_pv == 1)
            {
               prediction = j + 1;
            }
         }
         predict_result[0][count] = prediction;
         wait();
         count += 1;
      }
      count = 0;

      // 4-phase handshake with output, output current user's result
      predict_done.write(true);
      do { wait(); }
      while (!output_start.read());
      predict_done.write(false);
      do { wait(); }
      while (output_start.read());

      // This implementation is still computing debayer at once
      user += 1;
   }
}

void rbm::store()
{
RESET_STORE:
   data_out.reset_put();
   wr_length.write(0);
   wr_index.write(0);
   wr_request.write(false);

   output_start.write(false);
   output_done.write(true);
   done.write(false);

   do {wait();}
   while(!init_done.read());

   const u16 ntu = num_testusers.read();
   const u16 nm = num_movies.read();

   int index = 0;
   u8 rating;

STORE_OUTPUT_WHILE:
   while(true)
   {

      if (index == ntu)
      {
         // RBM Done (need a reset)
         done.write(true);
         do { wait(); }
         while(true);
      }

      // 4-phase handshake with prediction
      do { wait(); }
      while (!predict_done.read());
      output_start.write(true);
      do { wait(); }
      while (predict_done.read());
      output_start.write(false);
      output_done.write(false);

      unsigned dma_index = nm * (unsigned) index;
      wr_index.write(dma_index);
      wr_length.write(num_movies.read());
      index += 1;

      // Send DMA request
      wr_request.write(true);
      do { wait(); }
      while(!wr_grant.read());
      wr_request.write(false);

   STORE_SINGLE_LOOP:
      for (u16 i = 0; i < NUM_MOVIE_MAX; i++)
      {
         if (i == nm)
            break;
         rating = predict_result[0][i];
         wait();
         data_out.put((u32)rating);

      }

      output_done.write(true);

   }
}

#ifdef __CTOS__
SC_MODULE_EXPORT(rbm)
#endif




