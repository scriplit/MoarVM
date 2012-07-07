#!nqp
use MASTTesting;

plan(9);

sub array_type($frame) {
    my @ins := $frame.instructions;
    my $r0 := local($frame, str);
    my $r1 := local($frame, NQPMu);
    my $r2 := local($frame, NQPMu);
    op(@ins, 'const_s', $r0, sval('MVMArray'));
    op(@ins, 'knowhow', $r1);
    op(@ins, 'findmeth', $r2, $r1, sval('new_type'));
    call(@ins, $r2, [$Arg::obj, $Arg::named +| $Arg::str], $r1, sval('repr'), $r0, :result($r1));
    $r1
}

mast_frame_output_is(-> $frame, @ins, $cu {
        my $at := array_type($frame);
        my $r0 := local($frame, NQPMu);
        my $r1 := local($frame, int);
        op(@ins, 'create', $r0, $at);
        op(@ins, 'elemspos', $r1, $r0);
        op(@ins, 'say_i', $r1);
        op(@ins, 'return');
    },
    "0\n",
    "New array has zero elements");

mast_frame_output_is(-> $frame, @ins, $cu {
        my $at := array_type($frame);
        my $r0 := local($frame, NQPMu);
        my $r1 := local($frame, int);
        my $r2 := local($frame, int);
        op(@ins, 'create', $r0, $at);
        op(@ins, 'const_i64', $r2, ival(0));
        op(@ins, 'bindpos_o', $r0, $r2, $r0);
        op(@ins, 'elemspos', $r1, $r0);
        op(@ins, 'say_i', $r1);
        op(@ins, 'const_i64', $r2, ival(1));
        op(@ins, 'bindpos_o', $r0, $r2, $r0);
        op(@ins, 'elemspos', $r1, $r0);
        op(@ins, 'say_i', $r1);
        op(@ins, 'return');
    },
    "1\n2\n",
    "Adding elements increases element count");

mast_frame_output_is(-> $frame, @ins, $cu {
        my $at := array_type($frame);
        my $r0 := local($frame, NQPMu);
        my $r1 := local($frame, int);
        my $r2 := local($frame, int);
        my $r3 := local($frame, NQPMu);
        my $r4 := local($frame, NQPMu);
        my $r5 := local($frame, NQPMu);
        op(@ins, 'create', $r0, $at);
        op(@ins, 'create', $r3, $at);
        op(@ins, 'create', $r4, $at);
        op(@ins, 'const_i64', $r2, ival(0));
        op(@ins, 'bindpos_o', $r0, $r2, $r3);
        op(@ins, 'const_i64', $r2, ival(1));
        op(@ins, 'bindpos_o', $r0, $r2, $r4);
        op(@ins, 'const_i64', $r2, ival(0));
        op(@ins, 'atpos_o', $r5, $r0, $r2);
        op(@ins, 'eqaddr', $r1, $r5, $r3);
        op(@ins, 'say_i', $r1);
        op(@ins, 'eqaddr', $r1, $r5, $r4);
        op(@ins, 'say_i', $r1);
        op(@ins, 'const_i64', $r2, ival(1));
        op(@ins, 'atpos_o', $r5, $r0, $r2);
        op(@ins, 'eqaddr', $r1, $r5, $r3);
        op(@ins, 'say_i', $r1);
        op(@ins, 'eqaddr', $r1, $r5, $r4);
        op(@ins, 'say_i', $r1);
        op(@ins, 'return');
    },
    "1\n0\n0\n1\n",
    "Can retrieve items by index");

mast_frame_output_is(-> $frame, @ins, $cu {
        my $at := array_type($frame);
        my $r0 := local($frame, NQPMu);
        my $r1 := local($frame, int);
        my $r2 := local($frame, int);
        my $r3 := local($frame, NQPMu);
        my $r4 := local($frame, NQPMu);
        my $r5 := local($frame, NQPMu);
        op(@ins, 'create', $r0, $at);
        op(@ins, 'create', $r3, $at);
        op(@ins, 'create', $r4, $at);
        op(@ins, 'push_o', $r0, $r3);
        op(@ins, 'push_o', $r0, $r4);
        op(@ins, 'elemspos', $r2, $r0);
        op(@ins, 'say_i', $r2);
        op(@ins, 'const_i64', $r2, ival(0));
        op(@ins, 'atpos_o', $r5, $r0, $r2);
        op(@ins, 'eqaddr', $r1, $r5, $r3);
        op(@ins, 'say_i', $r1);
        op(@ins, 'eqaddr', $r1, $r5, $r4);
        op(@ins, 'say_i', $r1);
        op(@ins, 'const_i64', $r2, ival(1));
        op(@ins, 'atpos_o', $r5, $r0, $r2);
        op(@ins, 'eqaddr', $r1, $r5, $r3);
        op(@ins, 'say_i', $r1);
        op(@ins, 'eqaddr', $r1, $r5, $r4);
        op(@ins, 'say_i', $r1);
        op(@ins, 'return');
    },
    "2\n1\n0\n0\n1\n",
    "can push");

mast_frame_output_is(-> $frame, @ins, $cu {
        my $at := array_type($frame);
        my $r0 := local($frame, NQPMu);
        my $r1 := local($frame, int);
        my $r2 := local($frame, int);
        my $r3 := local($frame, NQPMu);
        my $r4 := local($frame, NQPMu);
        my $r5 := local($frame, NQPMu);
        op(@ins, 'create', $r0, $at);
        op(@ins, 'create', $r3, $at);
        op(@ins, 'create', $r4, $at);
        op(@ins, 'unshift_o', $r0, $r3);
        op(@ins, 'unshift_o', $r0, $r4);
        op(@ins, 'elemspos', $r2, $r0);
        op(@ins, 'say_i', $r2);
        op(@ins, 'const_i64', $r2, ival(0));
        op(@ins, 'atpos_o', $r5, $r0, $r2);
        op(@ins, 'eqaddr', $r1, $r5, $r3);
        op(@ins, 'say_i', $r1);
        op(@ins, 'eqaddr', $r1, $r5, $r4);
        op(@ins, 'say_i', $r1);
        op(@ins, 'const_i64', $r2, ival(1));
        op(@ins, 'atpos_o', $r5, $r0, $r2);
        op(@ins, 'eqaddr', $r1, $r5, $r3);
        op(@ins, 'say_i', $r1);
        op(@ins, 'eqaddr', $r1, $r5, $r4);
        op(@ins, 'say_i', $r1);
        op(@ins, 'return');
    },
    "2\n0\n1\n1\n0\n",
    "can unshift");

mast_frame_output_is(-> $frame, @ins, $cu {
        my $at := array_type($frame);
        my $r0 := local($frame, NQPMu);
        my $r1 := local($frame, int);
        my $r2 := local($frame, int);
        my $r3 := local($frame, NQPMu);
        my $r4 := local($frame, NQPMu);
        my $r5 := local($frame, NQPMu);
        op(@ins, 'create', $r0, $at);
        op(@ins, 'create', $r3, $at);
        op(@ins, 'create', $r4, $at);
        op(@ins, 'const_i64', $r2, ival(0));
        op(@ins, 'bindpos_o', $r0, $r2, $r3);
        op(@ins, 'const_i64', $r2, ival(1));
        op(@ins, 'bindpos_o', $r0, $r2, $r4);
        op(@ins, 'pop_o', $r5, $r0);
        op(@ins, 'eqaddr', $r1, $r5, $r3);
        op(@ins, 'say_i', $r1);
        op(@ins, 'eqaddr', $r1, $r5, $r4);
        op(@ins, 'say_i', $r1);
        op(@ins, 'pop_o', $r5, $r0);
        op(@ins, 'eqaddr', $r1, $r5, $r3);
        op(@ins, 'say_i', $r1);
        op(@ins, 'eqaddr', $r1, $r5, $r4);
        op(@ins, 'say_i', $r1);
        op(@ins, 'elemspos', $r2, $r0);
        op(@ins, 'say_i', $r2);
        op(@ins, 'return');
    },
    "0\n1\n1\n0\n0\n",
    "Can pop");

mast_frame_output_is(-> $frame, @ins, $cu {
        my $at := array_type($frame);
        my $r0 := local($frame, NQPMu);
        my $r1 := local($frame, int);
        my $r2 := local($frame, int);
        my $r3 := local($frame, NQPMu);
        my $r4 := local($frame, NQPMu);
        my $r5 := local($frame, NQPMu);
        op(@ins, 'create', $r0, $at);
        op(@ins, 'create', $r3, $at);
        op(@ins, 'create', $r4, $at);
        op(@ins, 'const_i64', $r2, ival(0));
        op(@ins, 'bindpos_o', $r0, $r2, $r3);
        op(@ins, 'const_i64', $r2, ival(1));
        op(@ins, 'bindpos_o', $r0, $r2, $r4);
        op(@ins, 'shift_o', $r5, $r0);
        op(@ins, 'eqaddr', $r1, $r5, $r3);
        op(@ins, 'say_i', $r1);
        op(@ins, 'eqaddr', $r1, $r5, $r4);
        op(@ins, 'say_i', $r1);
        op(@ins, 'shift_o', $r5, $r0);
        op(@ins, 'eqaddr', $r1, $r5, $r3);
        op(@ins, 'say_i', $r1);
        op(@ins, 'eqaddr', $r1, $r5, $r4);
        op(@ins, 'say_i', $r1);
        op(@ins, 'elemspos', $r2, $r0);
        op(@ins, 'say_i', $r2);
        op(@ins, 'return');
    },
    "1\n0\n0\n1\n0\n",
    "Can shift");

mast_frame_output_is(-> $frame, @ins, $cu {
        my $at := array_type($frame);
        my $r0 := local($frame, NQPMu);
        my $r1 := local($frame, int);
        my $r2 := local($frame, int);
        my $r3 := local($frame, NQPMu);
        my $r4 := local($frame, NQPMu);
        my $r5 := local($frame, NQPMu);
        op(@ins, 'create', $r0, $at);
        op(@ins, 'create', $r3, $at);
        op(@ins, 'create', $r4, $at);
        op(@ins, 'push_o', $r0, $r3);
        op(@ins, 'push_o', $r0, $r4);
        op(@ins, 'elemspos', $r2, $r0);
        op(@ins, 'say_i', $r2);
        op(@ins, 'const_i64', $r2, ival(0));
        op(@ins, 'setelemspos', $r0, $r2);
        op(@ins, 'elemspos', $r2, $r0);
        op(@ins, 'say_i', $r2);
        op(@ins, 'return');
    },
    "2\n0\n",
    "can clear all elements by setting elements to 0");

mast_frame_output_is(-> $frame, @ins, $cu {
        my $at := array_type($frame);
        my $a1 := local($frame, NQPMu);
        my $a1_0 := local($frame, NQPMu);
        my $a1_1 := local($frame, NQPMu);
        my $a2 := local($frame, NQPMu);
        my $a2_0 := local($frame, NQPMu);
        my $a2_1 := local($frame, NQPMu);
        my $i := local($frame, int);
        my $t := local($frame, NQPMu);
        my $offset := local($frame, int);
        my $count := local($frame, int);
        
        # First array.
        op(@ins, 'create', $a1, $at);
        op(@ins, 'create', $a1_0, $at);
        op(@ins, 'create', $a1_1, $at);
        op(@ins, 'push_o', $a1, $a1_0);
        op(@ins, 'push_o', $a1, $a1_1);
        
        # Second array.
        op(@ins, 'create', $a2, $at);
        op(@ins, 'create', $a2_0, $at);
        op(@ins, 'create', $a2_1, $at);
        op(@ins, 'push_o', $a2, $a2_0);
        op(@ins, 'push_o', $a2, $a2_1);
        
        # Splice second into middle of first.
        op(@ins, 'const_i64', $offset, ival(1));
        op(@ins, 'const_i64', $count, ival(0));
        op(@ins, 'splice', $a1, $a2, $offset, $count);
        
        # Emit elements.
        op(@ins, 'elemspos', $i, $a1);
        op(@ins, 'say_i', $i);
        
        # Check they are the expected values.
        op(@ins, 'shift_o', $t, $a1);
        op(@ins, 'eqaddr', $i, $t, $a1_0);
        op(@ins, 'say_i', $i);
        op(@ins, 'shift_o', $t, $a1);
        op(@ins, 'eqaddr', $i, $t, $a1_1);
        op(@ins, 'say_i', $i);
        op(@ins, 'shift_o', $t, $a1);
        op(@ins, 'eqaddr', $i, $t, $a2_0);
        op(@ins, 'say_i', $i);
        op(@ins, 'shift_o', $t, $a1);
        op(@ins, 'eqaddr', $i, $t, $a2_1);
        op(@ins, 'say_i', $i);
        
        op(@ins, 'return');
    },
    "4\n1\n1\n1\n1\n",
    "splice");
