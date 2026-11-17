import ifj26.*

@main
def main(): Unit =
  val x = 4
  val big = if x > 3 then true else false
  if big then
    print("big")
  else
    print("small")
