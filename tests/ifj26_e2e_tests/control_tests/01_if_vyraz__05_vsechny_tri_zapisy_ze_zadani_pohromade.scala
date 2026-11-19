import ifj26.*

@main
def main(): Unit =
  var x = -5
  val absolute = if x < 0 then 0 - x else x // v 'absolute' je hodnota 5
  // ukázky různých kombinací jednořádkových těl a bloků
  val abs2 = if x < 0 then
    0 - x
  else
    x
  //
  val abs3 = if x < 0 then 0 - x else
      x
  print(absolute)
  print(abs2)
  print(abs3)
