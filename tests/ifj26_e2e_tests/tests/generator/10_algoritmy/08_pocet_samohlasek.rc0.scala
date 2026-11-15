import ifj26.*

@main def main(): Unit =
  val s = "programming language"
  val n = strLength(s)
  var i = 0
  var cnt = 0
  while i < n do
    val c = ord(s, i)
    if c == 97 then
      cnt = cnt + 1
    else
      cnt = cnt
    if c == 101 then
      cnt = cnt + 1
    else
      cnt = cnt
    if c == 105 then
      cnt = cnt + 1
    else
      cnt = cnt
    if c == 111 then
      cnt = cnt + 1
    else
      cnt = cnt
    if c == 117 then
      cnt = cnt + 1
    else
      cnt = cnt
    i = i + 1
  print(cnt)
  print("\n")
