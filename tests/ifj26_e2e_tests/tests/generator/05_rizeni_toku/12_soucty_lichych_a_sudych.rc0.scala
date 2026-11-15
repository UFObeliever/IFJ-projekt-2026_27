import ifj26.*

@main def main(): Unit =
  var i = 1
  var odd = 0
  var even = 0
  while i <= 10 do
    val r = i - i / 2 * 2
    if r == 1 then
      odd = odd + i
    else
      even = even + i
    i = i + 1
  print(odd)
  print("\n")
  print(even)
  print("\n")
