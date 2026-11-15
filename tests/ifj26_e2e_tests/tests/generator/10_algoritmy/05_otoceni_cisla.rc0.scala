import ifj26.*

@main def main(): Unit =
  var n = 12345
  var rev = 0
  while n > 0 do
    val d = n - n / 10 * 10
    rev = rev * 10 + d
    n = n / 10
  print(rev)
  print("\n")
