import ifj26.*

@main def main(): Unit =
  var i = 1
  var cnt = 0
  var sum = 0
  while i <= 20 do
    val r = i - i / 2 * 2
    if r == 0 then
      cnt = cnt + 1
      sum = sum + i
    else
      cnt = cnt
    i = i + 1
  print(cnt)
  print("\n")
  print(sum)
  print("\n")
