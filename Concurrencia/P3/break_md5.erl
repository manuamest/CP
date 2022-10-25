-module(break_md5).
-define(PASS_LEN, 6).
-define(UPDATE_BAR_GAP, 1000).
-define(BAR_SIZE, 40).
-define(PRO_NUM, 7).

-export([break_md5s/1,
         break_md5/1,
         pass_to_num/1,
         num_to_pass/1,
         num_to_hex_string/1,
         hex_string_to_num/1
        ]).

-export([progress_loop/2]).
-export([break_md5/5]).
-export([init_pro/7]).

% Base ^ Exp

pow_aux(_Base, Pow, 0) ->
    Pow;
pow_aux(Base, Pow, Exp) when Exp rem 2 == 0 ->
    pow_aux(Base*Base, Pow, Exp div 2);
pow_aux(Base, Pow, Exp) ->
    pow_aux(Base, Base * Pow, Exp - 1).

pow(Base, Exp) -> pow_aux(Base, 1, Exp).

%% Number to password and back conversion

num_to_pass_aux(_N, 0, Pass) -> Pass;
num_to_pass_aux(N, Digit, Pass) ->
    num_to_pass_aux(N div 26, Digit - 1, [$a + N rem 26 | Pass]).

num_to_pass(N) -> num_to_pass_aux(N, ?PASS_LEN, []).

pass_to_num(Pass) ->
    lists:foldl(fun (C, Num) -> Num * 26 + C - $a end, 0, Pass).

%% Hex string to Number

hex_char_to_int(N) ->
    if (N >= $0) and (N =< $9) -> N - $0;
       (N >= $a) and (N =< $f) -> N - $a + 10;
       (N >= $A) and (N =< $F) -> N - $A + 10;
       true                    -> throw({not_hex, [N]})
    end.

int_to_hex_char(N) ->
    if (N >= 0)  and (N < 10) -> $0 + N;
       (N >= 10) and (N < 16) -> $A + (N - 10);
       true                   -> throw({out_of_range, N})
    end.

hex_string_to_num(Hex_Str) ->
    lists:foldl(fun(Hex, Num) -> Num*16 + hex_char_to_int(Hex) end, 0, Hex_Str).

num_to_hex_string_aux(0, Str) -> Str;
num_to_hex_string_aux(N, Str) ->
    num_to_hex_string_aux(N div 16,
                          [int_to_hex_char(N rem 16) | Str]).

num_to_hex_string(0) -> "0";
num_to_hex_string(N) -> num_to_hex_string_aux(N, []).



%% Progress bar runs in its own process

progress_loop(N, Bound) ->
    receive
        {stop, Pid} -> 
            io:fwrite("\n"),
            Pid ! stop;
        {stop_hashes, Hashes, Pid} -> 
            Pid ! {stop_hashes, Hashes},
            ok;
        {progress_report, Checked} ->
            N2 = N + Checked,
            Full_N = N2 * ?BAR_SIZE div Bound,
            Full = lists:duplicate(Full_N, $=),
            Empty = lists:duplicate(?BAR_SIZE - Full_N, $-),
            receive
                T3 ->
                    io:format("\r[~s~s] ~.2f% ~b", [Full, Empty, N2/Bound*100, T3]), % printea barra de progreso e velocidade
                    progress_loop(N2, Bound)
                end
    end.

%% break_md5/2 iterates checking the possible passwords

break_md5([], _, _, _, Pid) ->
    Pid ! finished,
    ok;
break_md5(Target_Hash, N, N, _, Pid) ->
    Pid ! {not_found, Target_Hash},  % Checked every possible password
    ok;
break_md5(Target_Hash, N, Bound, Progress_Pid, Pid) ->
    receive
        {del, New_Hashes} ->
            break_md5(New_Hashes, N , Bound, Progress_Pid, Pid);
        stop -> ok
    after 0 ->
        if N rem ?UPDATE_BAR_GAP == 0 ->
                Progress_Pid ! {progress_report, ?UPDATE_BAR_GAP};
            true ->
                ok
        end,


    T1 = erlang:monotonic_time(microsecond), 

    Pass = num_to_pass(N),
    Hash = crypto:hash(md5, Pass),
    Num_Hash = binary:decode_unsigned(Hash),

    T2 = erlang:monotonic_time(microsecond),
    T3 = T2 - T1, % Calculamos o tempo transcurrido entre o cálculo de cada hash
    Progress_Pid ! T3, % Enviámoslle T3 á progress_loop


    case lists:member(Num_Hash, Target_Hash) of % Comparamos se o hash actual está dentro da lista dos has que buscamos
        true ->
            io:format("\e[2K\r~.16B: ~s~n", [Num_Hash, Pass]), % Hash atopado
            Pid ! {find, lists:delete(Num_Hash, Target_Hash)}, 
            break_md5(lists:delete(Num_Hash, Target_Hash), N+1, Bound, Progress_Pid, Pid); % Eliminámolo da lista de hashes a buscar

        false ->
            break_md5(Target_Hash,  N+1, Bound, Progress_Pid, Pid)
    end
end.

init_pro(0, Num_Hashes, X, Bound, Progress_Pid, N, List_Pid) ->
    receive
        {stop_hashes, Hashes} -> {not_found, Hashes};
        stop -> ok;
        {find, Hashes} ->
            Fun = fun(Pid_Aux) -> Pid_Aux ! {del, Hashes} end,
            lists:foreach(Fun, List_Pid),
            init_pro(0, Num_Hashes, X, Bound, Progress_Pid, N, List_Pid);
        finished ->
            if N == ?PRO_NUM ->
                Progress_Pid ! {stop, self()},
                init_pro(0, Num_Hashes, X, Bound, Progress_Pid, N, List_Pid);
            true ->
                init_pro(0, Num_Hashes, 1, Bound, Progress_Pid, N+1, List_Pid)
            end;
        {not_found, Hashes} ->
            if N == ?PRO_NUM ->
                Progress_Pid ! {stop_hashes, Hashes, self()},
                init_pro(0, Num_Hashes, X, Bound, Progress_Pid, N, List_Pid);
            true ->
                init_pro(0, Num_Hashes, 1, Bound, Progress_Pid, N+1, List_Pid)
            end
    end;

init_pro(N, Num_Hashes, X, Bound, Progress_Pid, Y, List_Pid) ->
    Initial_Pro = Bound div ?PRO_NUM * (N-1),
    Final_Pro = Bound div ?PRO_NUM * N,
    Pid = spawn(?MODULE, break_md5,[Num_Hashes, Initial_Pro, Final_Pro, Progress_Pid, self()]), % Crea os novos procesos
    init_pro(N-1, Num_Hashes, X, Bound, Progress_Pid, Y, [Pid | List_Pid]).

%% Break a hash

break_md5s(Hashes) ->
    List_Pid = [],
    Bound = pow(26, ?PASS_LEN),
    Progress_Pid = spawn(?MODULE, progress_loop, [0, Bound]),
    Num_Hashes = lists:map(fun hex_string_to_num/1, Hashes),
    Res = init_pro(?PRO_NUM, Num_Hashes, 0, Bound, Progress_Pid, 1, List_Pid),
    Progress_Pid ! stop,
    Res.

break_md5(Hash) -> break_md5s([Hash]). % Se só metemos un hash a buscar metémolo nunha lista a el só
