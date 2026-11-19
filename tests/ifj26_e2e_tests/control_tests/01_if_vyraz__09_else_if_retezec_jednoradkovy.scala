import ifj26.*

@main
def main(): Unit =
  val a = -4
  val b = 0
  val c = 9
  val r1 = if a < 0 then -1 else if a == 0 then 0 else 1
  val r2 = if b < 0 then -1 else if b == 0 then 0 else 1
  val r3 = if c < 0 then -1 else if c == 0 then 0 else 1
  print(r1)
  print(" ")
  print(r2)
  print(" ")
  print(r3)
