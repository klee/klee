%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@stdout = external global %struct._IO_FILE*, align 8
@.str = private unnamed_addr constant [11 x i8] c"abcdefgh!\0A\00", align 1
@fwrite = alias i64 (i8*, i64, i64, %struct._IO_FILE*), i64 (i8*, i64, i64, %struct._IO_FILE*)* @__fwrite_alias

define i64 @__fwrite_alias(i8*, i64, i64, %struct._IO_FILE*) {
  ret i64 0
}

define void @test() {
  %1 = load %struct._IO_FILE*, %struct._IO_FILE** @stdout, align 8
  %2 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* %1, i8* getelementptr inbounds ([11 x i8], [11 x i8]* @.str, i32 0, i32 0))
  ret void
}

declare i32 @fprintf(%struct._IO_FILE*, i8*, ...)
