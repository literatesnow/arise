on 1:TEXT:*:#:{ $arise(%arise_text,$nick • $_userprefix $+ $chan,$1-) }
on 1:TEXT:*:?:{ $arise(%arise_priv,$nick $_pr(private),$1-) }
on 1:ACTION:*:#:{ $arise(%arise_text,$nick • $_userprefix $+ $chan,* $1-,) }
on 1:ACTION:*:?:{ $arise(%arise_priv,$nick $_pr(private),* $1-) }
on 1:NOTICE:*:#:{ $arise(%arise_text,$nick • $_userprefix $+ $chan,> $1-) }
on 1:NOTICE:*:?:{ $arise(%arise_priv,$nick $_pr(private),> $1-) }
on 1:JOIN:#:{ $arise(%arise_info,$nick joins $chan,$gettok($address($nick,5),2,33)) }
on 1:PART:#:{ $arise(%arise_info,$nick parts $chan,$iif($1,$1-,bye)) }
on 1:KICK:#:{ $arise($iif($knick == $me,%arise_self,%arise_info),$nick • $_userprefix $+ $chan,Kicks $knick $_pr($1-)) }
on 1:NICK:{ $arise(%arise_info,$nick • $iif($network,$network,$server),New nick: $newnick) }
on 1:QUIT:{ $arise(%arise_info,$nick quits $iif($network,$network,$server),$iif($1,$1-,Signed Off)) }
on 1:TOPIC:#:{ $arise(%arise_info,$nick • $_userprefix $+ $chan,Changes topic: $iif($1,$1-,nothing)) }
on 1:INVITE:#:{ $arise(%arise_info,$nick,Invites you into $chan) }
on 1:DISCONNECT:{ $arise(%arise_serv,$iif($network,$network,$server),Disconnected) }
on 1:CONNECT:{ $arise(%arise_serv,$iif($network,$network,$server),Connected $_pr($nick)) }
on 1:ERROR:*:{ $arise(%arise_serv,$iif($network,$network,$server),Error: $1-) }
on 1:RAWMODE:#:{ if ($_showmode($1)) { $arise(%arise_info,$nick • $_userprefix $+ $chan, Sets mode: $1-) } }

alias -l arise { if (%arise_state) { var %title = $replace($strip($2,burc),\,\\,_,\_) | var %body = $replace($strip($3,burc),\,\\,_,\_) | dll arise.dll ShowPopup %title $+ _ $+ %body $+ _ $+ $1 } }
alias -l _pr { return $chr(40) $+ $1 $+ $chr(41) }
alias -l _userprefix { if ($nick isreg $chan) { return $null } | if ($nick isop $chan) { return @ } | if ($nick ishop $chan) { return $chr(37) } | if ($nick isvoice $chan) { return + } }
alias -l _showmode { if (($nick == ChanServ) && (l isin $1)) { return 0 } | else { return 1 } }
alias ar { if ($1 == on) { set %arise_state 1 } | elseif ($1 == off) { set %arise_state 0 } | elseif (%arise_state) { set %arise_state 0 } | else { set %arise_state 1 } | echo Popups $iif(%arise_state,on,off) }
alias arise_example {
  var %v = $dll(arise.dll,Version,)
  $arise(%arise_serv,$gettok(%v,1,45),$gettok(%v,2,45))
  $arise(%arise_priv,These are examples - anything goes!,$gettok(%v,3,45))
  $arise(%arise_serv,Local IRC,Connected $_pr($me))
  $arise(%arise_info,$me joins #foo,~alpha@example.com)
  $arise(%arise_priv,ChanServ $_pr(private),Welcome to #foo! Word of the day is: expunged)
  $arise(%arise_info,ChanServ • @#foo,Sets mode: +o $me)
  $arise(%arise_text,Guest3985 • #foo,your all suck lamerz)
  $arise(%arise_text,Guest3985 • #foo,lolol)
  $arise(%arise_info,$me • @#foo,Sets mode: -o+b Guest3985 *!*anon@10.245.62.10)
  $arise(%arise_info,$me • @#foo,Kicks Guest3985 $_pr(you're))
  $arise(%arise_serv,Local IRC,Disconnected)
}

on 1:LOAD:{
  var %v = $dll(arise.dll,Version,)
  echo -ts $iif(%v,%v,Arise by bliP - http://nisda.net)
  set %arise_text Verdana_8_2_0,0,0_Verdana_8_1_0,0,0_174,219,255_7000_1_0_0_40_100_1_300
  set %arise_info Verdana_8_2_0,0,0_Verdana_8_1_0,0,0_191,255,191_5000_1_0_0_40_100_1_300
  set %arise_priv Verdana_8_2_0,0,0_Verdana_8_1_0,0,0_255,255,170_5000_1_0_0_40_100_1_300
  set %arise_serv Verdana_8_2_0,0,0_Verdana_8_1_0,0,0_255,196,196_5000_1_0_0_40_100_1_300
  set %arise_self Verdana_8_2_0,0,0_Verdana_8_1_0,0,0_255,196,196_5000_1_0_0_40_100_1_300
  set %arise_state 1
  echo -ts Script loaded.
}
